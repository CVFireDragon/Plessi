#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <switch.h>

int main(int argc, char **argv)
{
	
    gfxInitDefault();
	consoleInit(NULL);
	printf("Plessi v0.1 - By CVFD\n");
	printf("Press A to open Save Manager\n");
	printf("Press B to open NSP Installer\n");
	
	while(appletMainLoop())
    {
        hidScanInput();

        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		
		if(kDown & KEY_B)
		{

		}
		
		if(kDown & KEY_A)
		{

		}
		
		if(kDown & KEY_X)
		{
        
		}
		
		if(kDown & KEY_Y)
		{
        
		}
		
        if (kDown & KEY_PLUS) break;
		
        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
}