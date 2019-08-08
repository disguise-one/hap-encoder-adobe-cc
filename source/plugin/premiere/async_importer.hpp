#ifndef ASYNC_IMPORTER
#define ASYNC_IMPORTER

#include <future>
#include <mutex>

#include "importer.hpp"


// async_importer.hpp
//   support classes for async import

// Track an async frame read and decode
struct AsyncFrameRequest
{
    AsyncFrameRequest(int32_t frameNum_,
                      PPixHand adobeFrame_   // !!! hack
                     )
        : frameNum(frameNum_), adobeFrame(adobeFrame_)
    {
    }
    int32_t frameNum;

    PPixHand adobeFrame;
    std::promise<std::pair<PPixHand, bool> > adobeFramePromise;  // second is 'failed'
};

PREMPLUGENTRY xAsyncImportEntry(int inSelector, void *inParam);


// This object is created by an importer during imCreateAsyncImporter
class AsyncImporter
{
public:
    AsyncImporter(
        std::unique_ptr<AdobeImporterAPI> adobe,
        std::unique_ptr<Importer> importer,
        int32_t width, int32_t height,
        int32_t frameRateNumerator, int32_t frameRateDenominator);

    int32_t convertTimeToFrame(int64_t time_numerator, int64_t time_denominator) const;

    ~AsyncImporter();
    int OnInitiateAsyncRead(imSourceVideoRec& inSourceRec);
    int OnFlush();
    int OnGetFrame(imSourceVideoRec* inFrameRec);
private:
    std::unique_ptr<AdobeImporterAPI> adobe_;
    std::unique_ptr<Importer> importer_;

    int32_t width_;
    int32_t height_;
    int32_t frameRateNumerator_;
    int32_t frameRateDenominator_;

    std::mutex requestsLock_;
    std::vector<AsyncFrameRequest *> requests_;
};

#endif
