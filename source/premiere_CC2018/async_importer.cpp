#include "async_importer.hpp"

PREMPLUGENTRY xAsyncImportEntry(
    int		inSelector,
    void	*inParam)
{
    csSDK_int32			result = aiUnsupported;
    AsyncImporter	   *asyncImporter = 0;

    switch (inSelector)
    {
    case aiInitiateAsyncRead:
    {
        aiAsyncRequest* asyncRequest(reinterpret_cast<aiAsyncRequest*>(inParam));
        asyncImporter = reinterpret_cast<AsyncImporter*>(asyncRequest->inPrivateData);
        result = asyncImporter->OnInitiateAsyncRead(asyncRequest->inSourceRec);
        break;
    }
    case aiCancelAsyncRead:
    {
        return aiUnsupported;
        break;
    }
    case aiFlush:
    {
        asyncImporter = reinterpret_cast<AsyncImporter*>(inParam);
        result = asyncImporter->OnFlush();
        break;
    }
    case aiGetFrame:
    {
        imSourceVideoRec* getFrameRec(reinterpret_cast<imSourceVideoRec*>(inParam));
        asyncImporter = reinterpret_cast<AsyncImporter*>(getFrameRec->inPrivateData);
        result = asyncImporter->OnGetFrame(getFrameRec);
        break;
    }
    case aiClose:
    {
        asyncImporter = reinterpret_cast<AsyncImporter*>(inParam);
        delete asyncImporter;
        result = aiNoError;
        break;
    }
    }

    return result;
}

AsyncImporter::AsyncImporter(std::unique_ptr<AdobeImporterAPI> adobe,
                             std::unique_ptr<Importer> importer,
                             int32_t width, int32_t height,
                             int32_t frameRateNumerator, int32_t frameRateDenominator)
    : adobe_(std::move(adobe)), importer_(std::move(importer)),
      width_(width), height_(height),
      frameRateNumerator_(frameRateNumerator), frameRateDenominator_(frameRateDenominator)
{
}

AsyncImporter::~AsyncImporter()
{
}

int32_t AsyncImporter::convertTimeToFrame(double t) const
{
    return (int32_t)(t * frameRateNumerator_ / frameRateDenominator_);
}


int AsyncImporter::OnInitiateAsyncRead(imSourceVideoRec& inSourceRec)
{
    PrTime ticksPerSecond = 0;
    adobe_->TimeSuite->GetTicksPerSecond(&ticksPerSecond);
    double tFrame = (double)inSourceRec.inFrameTime / ticksPerSecond;
    int32_t iFrame = convertTimeToFrame(tFrame);

    // Get parameters for ReadFrameToBuffer()
    imFrameFormat *frameFormat = &inSourceRec.inFrameFormats[0];
    char *frameBuffer;
    prRect rect;
    if (frameFormat->inFrameWidth == 0 && frameFormat->inFrameHeight == 0)
    {
        frameFormat->inFrameWidth = width_;
        frameFormat->inFrameHeight = height_;
    }
    // Windows and MacOS have different definitions of Rects, so use the cross-platform prSetRect
    prSetRect(&rect, 0, 0, frameFormat->inFrameWidth, frameFormat->inFrameHeight);
    PPixHand adobeFrame;
    adobe_->PPixCreatorSuite->CreatePPix(&adobeFrame, PrPPixBufferAccess_ReadWrite, frameFormat->inPixelFormat, &rect);
    adobe_->PPixSuite->GetPixels(adobeFrame, PrPPixBufferAccess_ReadWrite, &frameBuffer);
    // If extra row padding is needed, add it
    csSDK_int32 stride;
    adobe_->PPixSuite->GetRowBytes(adobeFrame, &stride);

    csSDK_int32 bytesPerFrame = frameFormat->inFrameWidth * frameFormat->inFrameHeight * 4;

    auto frameRequest = new AsyncFrameRequest(iFrame, adobeFrame);

    // Track the request
    {
        std::lock_guard<std::mutex> guard(requestsLock_);
        requests_.push_back(frameRequest);
    }

    // Start loading it in memory asynchronously
    importer_->requestFrame(
        iFrame,
        [&, frameBuffer, stride, adobeFrame, frameRequest](const DecoderJob& job) {
            job.copyLocalToExternal((uint8_t *)frameBuffer, stride);
            {
                std::lock_guard<std::mutex> guard(requestsLock_);
                frameRequest->adobeFramePromise.set_value(adobeFrame);
            }
        },
        [&](const DecoderJob& job) {
            // on fail, shift it from requests_ to failedRequests_
            {
                std::lock_guard<std::mutex> guard(requestsLock_);
                std::swap(*std::find(requests_.begin(), requests_.end(), frameRequest), requests_.back());
                requests_.pop_back();
            }
            {
                std::lock_guard<std::mutex> guard(failedRequestsLock_);
                failedRequests_.push_back(frameRequest);
            }
        });

    // we've been called from Adobe's context (ie we're not in a worker thread) so assume it's
    // ok to cleanup failed requests
    serviceFailedRequests();

    return aiNoError;
}

void AsyncImporter::serviceFailedRequests()
{
    std::lock_guard<std::mutex> guard(failedRequestsLock_);
    for (auto &i : failedRequests_) {
        adobe_->PPixSuite->Dispose(i->adobeFrame);   // !!! hack
        delete i;
    }
}

int AsyncImporter::OnFlush()
{
    // any 'OnGetFrame' that hasn't been actioned at this point is screwed
    // !!! should maybe wait? if Adobe calls GetFrame, gets context switched, and we pull the rug out
    // !!! by killing the requests - would this cause a failure?

    // clear out requests
    std::vector<AsyncFrameRequest *> owned;
    {
        std::lock_guard<std::mutex> guard(requestsLock_);
        std::swap(owned, requests_);
    }

    // fail them
    {
        std::lock_guard<std::mutex> guard(failedRequestsLock_);
        failedRequests_.insert(failedRequests_.end(), owned.begin(), owned.end());
    }

    // we've been called from Adobe's context (ie we're not in a worker thread) so assume it's
    // ok to cleanup failed requests
    serviceFailedRequests();

    return aiNoError;
}


int AsyncImporter::OnGetFrame(imSourceVideoRec* inSourceRec)
{
    PrTime ticksPerSecond = 0;
    adobe_->TimeSuite->GetTicksPerSecond(&ticksPerSecond);
    double tFrame = (double)inSourceRec->inFrameTime / ticksPerSecond;
    int32_t iFrame = convertTimeToFrame(tFrame);

    std::unique_ptr<AsyncFrameRequest> frameRequest;

    {
        std::lock_guard<std::mutex> guard(requestsLock_);

        std::vector<AsyncFrameRequest *>::iterator foundRequest = std::find_if(
            requests_.begin(), requests_.end(),
            [&](auto r) { return r->frameNum == iFrame; });

        if (foundRequest == requests_.end()) {
            *(inSourceRec->outFrame) = NULL;
            return aiFrameNotFound;
        }

        frameRequest.reset(*foundRequest);

        // we own the request now, so remove it from the active list
        std::swap(*foundRequest, requests_.back());
        requests_.pop_back();
    }

    // wait for completion and return it to Adobe
    *(inSourceRec->outFrame) = frameRequest->adobeFramePromise.get_future().get();

    // we've been called from Adobe's context (ie we're not in a worker thread) so assume it's
    // ok to cleanup failed requests
    serviceFailedRequests();

    return aiNoError;
}