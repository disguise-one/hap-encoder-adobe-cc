#include <chrono>
#include <thread>

#include "export_settings.hpp"

ExportSettings::ExportSettings()
{
}

ExportSettings::~ExportSettings()
{

}

csSDK_int32 getPixelFormatSize(const PrFourCC subtype)
{
    // !!! wrong.
	return 4;
}

