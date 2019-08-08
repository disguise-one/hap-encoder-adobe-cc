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

int32_t AsyncImporter::convertTimeToFrame(int64_t time_numerator, int64_t time_denominator) const
{
    int64_t num = time_numerator * frameRateNumerator_;
    int64_t den = time_denominator * frameRateDenominator_;
    return (int32_t)(num / den);
}

int AsyncImporter::OnInitiateAsyncRead(imSourceVideoRec& inSourceRec)
{
    try {
        PrTime ticksPerSecond = 0;
        adobe_->TimeSuite->GetTicksPerSecond(&ticksPerSecond);
        int32_t iFrame = convertTimeToFrame(inSourceRec.inFrameTime, ticksPerSecond);

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
                    bool failed = false;
                    frameRequest->adobeFramePromise.set_value(std::make_pair(adobeFrame, failed));
                }
            },
            [&, adobeFrame, frameRequest](const DecoderJob& job) {
                {
                    bool failed = true;
                    frameRequest->adobeFramePromise.set_value(std::make_pair(adobeFrame, failed));
                }
            });
    }
    catch (...)
    {
        return aiUnknownError;
    }

    return aiNoError;
}

int AsyncImporter::OnFlush()
{
    try
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

        for (auto &request : owned)
        {
            std::pair<PPixHand, bool> completed = request->adobeFramePromise.get_future().get();
            adobe_->PPixSuite->Dispose(completed.first);   // !!! hack
            delete request;
        }
    }
    catch (...)
    {
        return aiUnknownError;
    }

    return aiNoError;
}


int AsyncImporter::OnGetFrame(imSourceVideoRec* inSourceRec)
{
    try
    {
        PrTime ticksPerSecond = 0;
        adobe_->TimeSuite->GetTicksPerSecond(&ticksPerSecond);
        int32_t iFrame = convertTimeToFrame(inSourceRec->inFrameTime, ticksPerSecond);

        AsyncFrameRequest *frameRequest;

        {
            std::lock_guard<std::mutex> guard(requestsLock_);

            std::vector<AsyncFrameRequest *>::iterator foundRequest = std::find_if(
                requests_.begin(), requests_.end(),
                [&](auto r) { return r->frameNum == iFrame; });

            if (foundRequest == requests_.end()) {
                *(inSourceRec->outFrame) = NULL;
                return aiFrameNotFound;
            }

            frameRequest = *foundRequest;

            // one way or another the request will be dispatched in this function
            std::swap(*foundRequest, requests_.back());
            requests_.pop_back();
        }

        // wait for completion and return it to Adobe
        std::pair<PPixHand, bool> completed = frameRequest->adobeFramePromise.get_future().get();
        delete frameRequest;

        bool failed = completed.second;
        if (failed)
        {
            adobe_->PPixSuite->Dispose(completed.first);
            return aiFrameNotFound;
        }
        else {

            *(inSourceRec->outFrame) = completed.first;
        }
    }
    catch (...)
    {
        return aiUnknownError;
    }

    return aiNoError;
}