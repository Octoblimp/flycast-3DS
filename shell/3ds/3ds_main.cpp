#ifndef LIBRETRO
#include <3ds.h>

#include "stdclass.h"
#include "log/LogManager.h"
#include "emulator.h"
#include "ui/mainui.h"
#include "oslib/directory.h"
#include "oslib/i18n.h"
#include "cfg/option.h"
#include "rend/osd.h"

#include <array>
#include <cstdio>

static PrintConsole bottomConsole;
static bool bottomConsoleInitialized = false;
static std::array<u64, 8> bottomVmuLastChangedTimestamps {};
static int bottomVmuLastIndex = -1;
static constexpr u32 VmuPixelBrightnessThreshold = 384;
static constexpr int MinLitPixelsForBlock = 2;

static bool vmuPixelOn(u32 pixel)
{
	const u32 r = pixel & 0xff;
	const u32 g = (pixel >> 8) & 0xff;
	const u32 b = (pixel >> 16) & 0xff;
	const u32 a = (pixel >> 24) & 0xff;
	return a != 0 && (r + g + b) >= VmuPixelBrightnessThreshold;
}

static int getActiveVmuIndex()
{
	for (int i = 0; i < 8; i++)
	{
		if (vmu_lcd_status[i])
			return i;
	}
	return -1;
}

static void drawBottomScreenVmu()
{
	if (!bottomConsoleInitialized)
		return;
	const int vmuIndex = config::FloatVMUs ? getActiveVmuIndex() : -1;
	if (vmuIndex == bottomVmuLastIndex
			&& (vmuIndex < 0 || bottomVmuLastChangedTimestamps[vmuIndex] == vmuLastChanged[vmuIndex]))
		return;

	consoleSelect(&bottomConsole);
	consoleClear();
	if (vmuIndex < 0)
	{
		std::printf("Flycast 3DS\n\nNo active VMU display.");
		bottomVmuLastIndex = vmuIndex;
		return;
	}

	std::printf("VMU %d (bottom)\n\n", vmuIndex + 1);
	for (int y = 0; y < 32; y += 2)
	{
		for (int x = 0; x < 48; x += 2)
		{
			const u32 p0 = vmu_lcd_data[vmuIndex][y * 48 + x];
			const u32 p1 = vmu_lcd_data[vmuIndex][y * 48 + x + 1];
			const u32 p2 = vmu_lcd_data[vmuIndex][(y + 1) * 48 + x];
			const u32 p3 = vmu_lcd_data[vmuIndex][(y + 1) * 48 + x + 1];
			const int litPixelCount = (int)vmuPixelOn(p0) + (int)vmuPixelOn(p1) + (int)vmuPixelOn(p2) + (int)vmuPixelOn(p3);
			std::putchar(litPixelCount >= MinLitPixelsForBlock ? '#' : ' ');
		}
		std::putchar('\n');
	}
	bottomVmuLastIndex = vmuIndex;
	bottomVmuLastChangedTimestamps[vmuIndex] = vmuLastChanged[vmuIndex];
}

int main(int argc, char *argv[])
{
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, &bottomConsole);
	bottomConsoleInitialized = true;

	Result romfsRc = romfsInit();
	if (R_FAILED(romfsRc))
		WARN_LOG(BOOT, "romfsInit failed: 0x%08lx", (unsigned long)romfsRc);

	LogManager::Init();
	i18n::init();

	flycast::mkdir("sdmc:/3ds/flycast", 0755);
	flycast::mkdir("sdmc:/3ds/flycast/data", 0755);
	set_user_config_dir("sdmc:/3ds/flycast/");
	set_user_data_dir("sdmc:/3ds/flycast/data/");

	add_system_config_dir("sdmc:/3ds/flycast/");
	add_system_config_dir("./");
	add_system_data_dir("sdmc:/3ds/flycast/data/");
	add_system_data_dir("./");
	add_system_data_dir("data/");

	// 3DS VMU default: keep VMU LCD overlays enabled in-game.
	config::FloatVMUs = true;

	if (flycast_init(argc, argv))
		die("Flycast initialization failed");

	try {
		mainui_loop();
	} catch (const std::exception& e) {
		ERROR_LOG(BOOT, "mainui_loop error: %s", e.what());
	} catch (...) {
		ERROR_LOG(BOOT, "mainui_loop unknown exception");
	}

	flycast_term();
	if (R_SUCCEEDED(romfsRc))
		romfsExit();
	gfxExit();

	return 0;
}

void os_DoEvents()
{
	hidScanInput();
	drawBottomScreenVmu();
	if (!aptMainLoop())
		dc_exit();
}

namespace hostfs
{

void saveScreenshot(const std::string&, const std::vector<u8>&)
{
	throw FlycastException(i18n::Ts("Not supported on 3DS"));
}

}
#endif
