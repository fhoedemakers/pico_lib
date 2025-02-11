/*
 * author : Shuichi TAKANO
 * since  : Sun Jun 20 2021 03:34:30
 */
#ifndef C0B8F27B_5134_63A8_332D_5F7EB7D8492C
#define C0B8F27B_5134_63A8_332D_5F7EB7D8492C

#include "config.h"
#include "timing.h"
#include "dma.h"
#include <hardware/pio.h>
#include <stdint.h>
#include <array>
#include <vector>
#include <util/queue.h>
#include <util/ring_buffer.h>

namespace dvi
{
    class DVI
    {
    public:
        using PixelType = uint16_t;
        using LineBuffer = std::vector<PixelType>;
        using AudioSample = std::array<int16_t, 2>;

    public:
        DVI(PIO pio, const Config *cfg, const Timing *timing);
        void registerIRQThisCore();
        void unregisterIRQThisCore();

        void start();
        void stop();

        //        void __not_in_flash_func(loopScanBuffer16bpp)();
        void __not_in_flash_func(loopScanBuffer15bpp)();

        void __not_in_flash_func(convertScanBuffer15bpp)();
        void __not_in_flash_func(convertScanBuffer12bpp)();
        void __not_in_flash_func(convertScanBuffer12bpp)(uint16_t line, uint16_t *buffer, size_t size);
        void __not_in_flash_func(convertScanBuffer12bppScaled16_7)(int srcPixelOfs, int dstPixelOfs, int dstPixels);
        void __not_in_flash_func(convertScanBuffer12bppScaled16_7)(int srcPixelOfs, int dstPixelOfs, int dstPixels, uint16_t line, uint16_t *buffer, size_t size);
        uint32_t getFrameCounter() const
        {
            return frameCounter_;
        }

        BlankSettings &getBlankSettings() { return blankSettings_; }
        void setScanLine(bool f) { enableScanLine_ = f; }

        LineBuffer *__not_in_flash_func(getLineBuffer)();
        void __not_in_flash_func(setLineBuffer)(int line, LineBuffer *);
        void __not_in_flash_func(waitForValidLine)();

        void setAudioFreq(int freq, int CTS, int N);
        void allocateAudioBuffer(size_t size);
        util::RingBuffer<AudioSample> &getAudioRingBuffer() { return audioSampleRing_; }

    protected:
        void initSerialiser();
        void enableSerialiser(bool enable = true);
        void allocateBuffers(const Timing *timing);
        void enableDataIsland();

        void __not_in_flash_func(advanceLine)();
        void __not_in_flash_func(updateDataPacket)();
        void __not_in_flash_func(dmaIRQHandler)();

        static void __not_in_flash_func(dmaIRQEntry)();

    private:
        PIO pio_;
        const Config *config_{};
        const Timing *timing_{};
        BlankSettings blankSettings_;
        bool enableDataIsland_ = false;
        bool enableScanLine_ = false;

        bool started_ = false;
        LineState lineState_{};
        int lineCounter_ = 0;

        DMA dma_;

        uint32_t frameCounter_ = 0;

        template <class T>
        struct ResultBuffer
        {
            int line{};
            T buffer{};
        };

        using TMDSBuffer = std::vector<uint32_t>;
        using ResultTMDSBuffer = ResultBuffer<TMDSBuffer *>;
        using ResultLineBuffer = ResultBuffer<LineBuffer *>;

        static inline constexpr size_t N_BUFFERS = 5;
        static inline constexpr size_t N_COLOR_CH = 3;

        TMDSBuffer tmdsBuffers_[N_BUFFERS];
        LineBuffer lineBuffers_[N_BUFFERS];

        util::Queue<ResultTMDSBuffer> validTMDSQueue_{N_BUFFERS};
        util::Queue<TMDSBuffer *> freeTMDSQueue_{N_BUFFERS};
        TMDSBuffer *curTMDSBuffer_{};
        TMDSBuffer *releaseTMDSBuffer_[2]{};

        util::Queue<ResultLineBuffer> validLineQueue_{N_BUFFERS};
        util::Queue<LineBuffer *> freeLineQueue_{N_BUFFERS};

        DataPacket aviInfoFrame_;
        DataPacket audioClockRegeneration_;
        DataPacket audioInfoFrame_;
        int audioFreq_ = 0;
        int samplesPerFrame_ = 0;
        int samplesPerLine16_ = 0;

        std::vector<AudioSample> audioSampleBuffer_;
        util::RingBuffer<AudioSample> audioSampleRing_;

        int leftAudioSampleCount_ = 0;
        int audioSamplePos_ = 0;
        int audioFrameCount_ = 0;

        int pendingAudioLineCount_ = 0;
    };
}

#endif /* C0B8F27B_5134_63A8_332D_5F7EB7D8492C */
