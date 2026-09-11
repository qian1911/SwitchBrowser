#include <switch.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    printf("\n\n  SwitchBrowser Test\n");
    printf("  ===================\n\n");
    printf("  If you see this text, the NRO works!\n\n");
    printf("  Press [+] to exit.\n");

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 key = padGetButtonsDown(&pad);

        if (key & HidNpadButton_Plus)
            break;

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
