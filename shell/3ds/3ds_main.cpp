#ifndef LIBRETRO
#include <3ds.h>

#include "stdclass.h"
#include "log/LogManager.h"
#include "emulator.h"
#include "ui/mainui.h"
#include "oslib/directory.h"
#include "oslib/i18n.h"
#include "cfg/option.h"

int main(int argc, char *argv[])
{
	romfsInit();

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

	// 3DS virtual VDU default: keep VMU LCD overlays enabled in-game.
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
	romfsExit();

	return 0;
}

void os_DoEvents()
{
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
