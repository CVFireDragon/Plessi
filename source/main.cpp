#include <stdio.h>
#include "main.h"
#include "autorcm.h"

//Tinfoil Start

#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdexcept>
#include <memory>
#include <switch.h>

#include "nx/ipc/tin_ipc.h"

#include "mode/mode.hpp"
#include "mode/install_extracted_mode.hpp"
#include "mode/install_nsp_mode.hpp"
#include "mode/delete_common_ticket_mode.hpp"
#include "mode/delete_personalized_ticket_mode.hpp"
#include "mode/network_install_mode.hpp"
#include "mode/usb_install_mode.hpp"
#include "mode/verify_nsp_mode.hpp"
#include "ui/framework/view.hpp"
#include "ui/framework/console_options_view.hpp"
#include "ui/install_view.hpp"

#include "debug.h"
#include "error.hpp"

#ifdef __cplusplus
extern "C" {
#endif
#include "tinfs.h"

//Tinfoil End

void userAppInit(void);
void userAppExit(void);

#ifdef __cplusplus
}
#endif

// TODO: Make custom error codes in some sort of logical fashion
// TODO: Create a proper logging setup, as well as a log viewing screen
// TODO: Validate NCAs
// TODO: Verify dumps, ncaids match sha256s, installation succeess, perform proper uninstallation on failure and prior to install

u8* g_framebuf;
u32 g_framebufWidth;
u32 g_framebufHeight;

bool g_shouldExit = false;
u8 firmwareMajorVersion;

// Below function copied from SwitchIdent (https://github.com/joel16/SwitchIdent)
void getFirmwareVersion() {
    if (R_FAILED(setsysInitialize()))
        fatalSimple(0xBEEB);

    SetSysFirmwareVersion ver;
    if (R_FAILED(setsysGetFirmwareVersion(&ver)))
        fatalSimple(0xBEEC);

    firmwareMajorVersion = ver.major;
    setsysExit();
}

long GetAvailableSpace(const char *path)
{
	struct statvfs stat;

	if (statvfs(path, &stat) != 0)
	{
		return -1;
	}
	return stat.f_bsize * stat.f_bavail;
}

void AutoRcmToggle()
{
	//Credits to Rei's Toolkit

	printf("Toggle AutoRcm...\n");

	FsStorage storage;

	int returnable = 0;

	int boot0_part = 0;

	Result r = fsOpenBisStorage(&storage, boot0_part);

	u64 size = 0;

	fsStorageGetSize(&storage, &size);

	if (size == 0)
	{
		return -1;
	}

	char *buf = (char *)malloc(size);
	r = fsStorageRead(&storage, 0, buf, size);
	for (int i = 0; i < 4; i++)
	{
		int off = BCT_KEY_OFF + i * BCT_SZ;
		buf[off] ^= RCM_XOR;
		if (buf[off] != 0xF7)
			returnable = 1;
	}

	fsStorageWrite(&storage, 0, buf, size);
	fsStorageClose(&storage);
	free(buf);
	printf("done! press home to exit");
	return returnable;
}

void AutoRcmMenu()
{
	consoleClear();
	AutoRcmMenuCalled = 1;

	printf("X to toggle AutoRcm or press home to exit\n");

	while (appletMainLoop())
	{
		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if (kDown & KEY_X) AutoRcmToggle();
		gfxFlushBuffers();
		gfxSwapBuffers();
	}
}

void userAppInit(void)
{
    getFirmwareVersion();

    if (R_FAILED(ncmextInitialize()))
        fatalSimple(0xBEEF);

    if (R_FAILED(ncmInitialize()))
        fatalSimple(0xBEE2);

    if (R_FAILED(nsInitialize()))
        fatalSimple(0xBEE3);

    if (R_FAILED(nsextInitialize()))
        fatalSimple(0xBEE4);

    if (R_FAILED(esInitialize()))
        fatalSimple(0xBEE5);

    if (R_FAILED(nifmInitialize()))
        fatalSimple(0xBEE6);

    if (R_FAILED(setInitialize()))
        fatalSimple(0xBEE7);

    if (R_FAILED(plInitialize()))
        fatalSimple(0xBEE8);

    if (R_FAILED(romfsInit()))
        fatalSimple(0xBEE9);

    if (firmwareMajorVersion > 4) {
        if (R_FAILED(usbCommsInitialize()))
            fatalSimple(0xBEEA);
    }

    // We initialize this inside ui_networkinstall_mode for normal users.
    #ifdef NXLINK_DEBUG
    socketInitializeDefault();
    nxLinkInitialize();
    #endif
}

void userAppExit(void)
{
    nifmExit();

    #ifdef NXLINK_DEBUG
    nxLinkExit();
    socketExit();
    #endif

    if (firmwareMajorVersion > 4) {
        usbCommsExit();
    }

    romfsExit();
    plExit();
    setExit();
    ncmextExit();
    ncmExit();
    nsExit();
    nsextExit();
    esExit();
}

void markForExit(void)
{
    g_shouldExit = true;
}

int main(int argc, char **argv)
{
	
	gfxInitDefault();
	consoleInit(NULL);
	printf("Plessi v0.1 - By CVFD\n");
	printf("Press A to Shutdown\n");
	printf("Press B to Reboot\n");
	printf("Press Y to open NSP Installer\n");
	printf("Press X for AutoRCM\n");
	
	while(appletMainLoop())
    {
        hidScanInput();

        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		if(kDown & KEY_B)
		{
        bpcInitialize();
        bpcRebootSystem();
		}
		
		if(kDown & KEY_A)
		{
        bpcInitialize();
        bpcShutdownSystem();
		}
		
		if(kDown & KEY_Y)
		{
    try
    {
        Result rc = 0;
        tin::ui::ViewManager& manager = tin::ui::ViewManager::Instance();

        gfxInitDefault();
        manager.m_printConsole = consoleInit(NULL);
        LOG_DEBUG("NXLink is active\n");

        g_framebuf = gfxGetFramebuffer(&g_framebufWidth, &g_framebufHeight);

        // Create the tinfoil directory and subdirs on the sd card if they don't already exist. 
        // These are used throughout the app without existance checks.
        if (R_FAILED(rc = createTinfoilDirs()))
        {
            printf("main: Failed to create tinfoil dirs. Error code: 0x%08x\n", rc);
            return 0;
        }
        
        tin::ui::Category titleManCat("Title Management");
        titleManCat.AddMode(std::move(std::make_unique<tin::ui::InstallNSPMode>()));
        titleManCat.AddMode(std::move(std::make_unique<tin::ui::VerifyNSPMode>()));
        titleManCat.AddMode(std::move(std::make_unique<tin::ui::InstallExtractedNSPMode>()));
        if (firmwareMajorVersion > 4) {
            titleManCat.AddMode(std::move(std::make_unique<tin::ui::USBInstallMode>()));
        }
        titleManCat.AddMode(std::move(std::make_unique<tin::ui::NetworkInstallMode>()));
        // TODO: Add uninstall and dump nsp

        tin::ui::Category tikManCat("Ticket Management");
        tikManCat.AddMode(std::move(std::make_unique<tin::ui::DeleteCommonTicketMode>()));
        tikManCat.AddMode(std::move(std::make_unique<tin::ui::DeletePersonalizedTicketMode>()));

        // TODO: Add install tik and cert, delete personalized ticket and view title keys

        auto mainView = std::make_unique<tin::ui::ConsoleOptionsView>();
        auto showUITesting = [&]()
        {
            auto installView = std::make_unique<tin::ui::InstallView>();
            manager.PushView(std::move(installView));
        };

        mainView->AddEntry("Main Menu", tin::ui::ConsoleEntrySelectType::HEADING, nullptr);
        mainView->AddEntry("", tin::ui::ConsoleEntrySelectType::NONE, nullptr);
        mainView->AddEntry(titleManCat.m_name, tin::ui::ConsoleEntrySelectType::SELECT, std::bind(&tin::ui::Category::OnSelected, &titleManCat));
        mainView->AddEntry("Install Information", tin::ui::ConsoleEntrySelectType::SELECT_INACTIVE, nullptr);
        mainView->AddEntry("Ticket Management", tin::ui::ConsoleEntrySelectType::SELECT, std::bind(&tin::ui::Category::OnSelected, &tikManCat));
        mainView->AddEntry("UI Testing", tin::ui::ConsoleEntrySelectType::SELECT, showUITesting);
        mainView->AddEntry("Exit", tin::ui::ConsoleEntrySelectType::SELECT, markForExit);
        
        manager.PushView(std::move(mainView));

        while (appletMainLoop() && !g_shouldExit)
        {
            hidScanInput();
            u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

            if (kDown)
                manager.ProcessInput(kDown);

            g_framebuf = gfxGetFramebuffer(&g_framebufWidth, &g_framebufHeight);

            manager.Update();

            gfxFlushBuffers();
            gfxSwapBuffers();
        }
    }
    catch (std::exception& e)
    {
        consoleClear();
        printf("An error occurred:\n%s\n\nPress any button to exit.", e.what());
        LOG_DEBUG("An error occurred:\n%s", e.what());

        u64 kDown = 0;

        while (!kDown)
        {
            hidScanInput();
            kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        }
    }
    catch (...)
    {
        consoleClear();
        printf("An unknown error occurred\n\nPress any button to exit.");
        LOG_DEBUG("An unknown error occurred:\n");

        u64 kDown = 0;

        while (!kDown)
        {
            hidScanInput();
            kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        }
    }
		}
		
		if(kDown & KEY_A)
		{
		AutoRcmMenu();
		}
		
        if (kDown & KEY_PLUS) break;
		
        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
}
