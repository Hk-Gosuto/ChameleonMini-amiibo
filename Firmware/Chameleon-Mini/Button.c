/*
 * Button.c
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 *
 *  ChangeLog
 *    2019-09-22    willok    Reverse switch function and log clearing function are added.
 *
 */

#include "Button.h"
#include "Random.h"
#include "Common.h"
#include "Settings.h"
#include "Memory.h"
#include "Map.h"
#include "Configuration.h"
#include "Terminal/CommandLine.h"
#include "Application/Application.h"

#define BUTTON_PORT PORTA
#define BUTTON_L PIN5_bm
#define BUTTON_R PIN6_bm
#define BUTTON_L_PINCTRL PIN5CTRL
#define BUTTON_R_PINCTRL PIN6CTRL
#define BUTTON_MASK (BUTTON_L | BUTTON_R)

#define LONG_PRESS_TICK_COUNT 10

static const MapEntryType PROGMEM ButtonActionMap[] = {
    {.Id = BUTTON_ACTION_NONE, .Text = "NONE"},
    {.Id = BUTTON_ACTION_UID_RANDOM, .Text = "UID_RANDOM"},
    {.Id = BUTTON_ACTION_UID_LEFT_INCREMENT, .Text = "UID_LEFT_INCREMENT"},
    {.Id = BUTTON_ACTION_UID_RIGHT_INCREMENT, .Text = "UID_RIGHT_INCREMENT"},
    {.Id = BUTTON_ACTION_UID_LEFT_DECREMENT, .Text = "UID_LEFT_DECREMENT"},
    {.Id = BUTTON_ACTION_UID_RIGHT_DECREMENT, .Text = "UID_RIGHT_DECREMENT"},
    {.Id = BUTTON_ACTION_CYCLE_SETTINGS, .Text = "CYCLE_SETTINGS"},
    {.Id = BUTTON_ACTION_CYCLE_SETTINGS_DEC, .Text = "CYCLE_SETTINGS_DEC"},
    {.Id = BUTTON_ACTION_STORE_MEM, .Text = "STORE_MEM"},
    {.Id = BUTTON_ACTION_RECALL_MEM, .Text = "RECALL_MEM"},
    {.Id = BUTTON_ACTION_TOGGLE_FIELD, .Text = "TOGGLE_FIELD"},
    {.Id = BUTTON_ACTION_STORE_LOG, .Text = "STORE_LOG"},
    {.Id = BUTTON_ACTION_CLEAR_LOG, .Text = "CLEAR_LOG"},
    {.Id = BUTTON_ACTION_CLONE, .Text = "CLONE"},
    {.Id = BUTTON_ACTION_CLONE_MFU, .Text = "CLONE_MFU"},
};

static void ExecuteButtonAction(ButtonActionEnum ButtonAction)
{
    uint8_t UidBuffer[32];

    switch (ButtonAction)
    {
    case BUTTON_ACTION_UID_RANDOM:
    {
        for (uint8_t i = 0; i < ActiveConfiguration.UidSize; i++)
        {
            UidBuffer[i] = RandomGetByte();
        }
        /* If we are using an ISO15 tag, the first byte needs to be E0 by standard */
        if (ActiveConfiguration.TagFamily == TAG_FAMILY_ISO15693)
        {
            UidBuffer[0] = 0xE0;
        }

        /* If we are using an NTAG21x tag, the first byte needs to be 04 by standard */
        if (ActiveConfiguration.TagFamily == TAG_FAMILY_ISO14443A && ActiveConfiguration.UidSize == NTAG21x_UID_SIZE)
        {
            UidBuffer[0] = 0x04;
            // unlock
            uint8_t Page2Buffer[2];
            Page2Buffer[0] = 0x00;
            Page2Buffer[1] = 0x00;
            MemoryWriteBlock(Page2Buffer, 0x02 * NTAG21x_PAGE_SIZE + 2, 2);

            uint8_t Page3Buffer[1];
            Page3Buffer[0] = 0xE1;
            MemoryWriteBlock(Page3Buffer, 0x03 * NTAG21x_PAGE_SIZE, 1);

            uint8_t Page82Buffer[4];
            Page82Buffer[0] = 0x00;
            Page82Buffer[1] = 0x00;
            Page82Buffer[2] = 0x00;
            Page82Buffer[3] = 0xBD;
            MemoryWriteBlock(Page82Buffer, 0x82 * NTAG21x_PAGE_SIZE, NTAG21x_PAGE_SIZE);

            uint8_t Page83Buffer[4];
            Page83Buffer[0] = 0x04;
            Page83Buffer[1] = 0x00;
            Page83Buffer[2] = 0x00;
            Page83Buffer[3] = 0xFF;
            MemoryWriteBlock(Page83Buffer, 0x83 * NTAG21x_PAGE_SIZE, NTAG21x_PAGE_SIZE);

            uint8_t Page84Buffer[4];
            Page84Buffer[0] = 0x00;
            Page84Buffer[1] = 0x05;
            Page84Buffer[2] = 0x00;
            Page84Buffer[3] = 0x00;
            MemoryWriteBlock(Page84Buffer, 0x84 * NTAG21x_PAGE_SIZE, NTAG21x_PAGE_SIZE);
        }

        ApplicationSetUid(UidBuffer);
        break;
    }

    case BUTTON_ACTION_UID_LEFT_INCREMENT:
    {
        uint8_t offset = 0;
#ifdef SUPPORT_UID7_FIX_MANUFACTURER_BYTE
        if (ActiveConfiguration.UidSize == 7)
        {
            offset = 1;
        }
#endif
        ApplicationGetUid(UidBuffer);
        bool Carry = 1;
        uint8_t i;

        for (i = offset; i < ActiveConfiguration.UidSize; i++)
        {
            if (Carry)
            {
                if (UidBuffer[i] == 0xFF)
                {
                    Carry = 1;
                }
                else
                {
                    Carry = 0;
                }

                UidBuffer[i] = (UidBuffer[i] + 1) & 0xFF;
            }
        }

        ApplicationSetUid(UidBuffer);
        break;
    }

    case BUTTON_ACTION_UID_RIGHT_INCREMENT:
    {
        ApplicationGetUid(UidBuffer);
        bool Carry = 1;
        uint8_t i = ActiveConfiguration.UidSize;

        while (i-- > 0)
        {
            if (Carry)
            {
                if (UidBuffer[i] == 0xFF)
                {
                    Carry = 1;
                }
                else
                {
                    Carry = 0;
                }

                UidBuffer[i] = (UidBuffer[i] + 1) & 0xFF;
            }
        }

        ApplicationSetUid(UidBuffer);
        break;
    }

    case BUTTON_ACTION_UID_LEFT_DECREMENT:
    {
        uint8_t offset = 0;
#ifdef SUPPORT_UID7_FIX_MANUFACTURER_BYTE
        if (ActiveConfiguration.UidSize == 7)
        {
            offset = 1;
        }
#endif
        ApplicationGetUid(UidBuffer);
        bool Carry = 1;
        uint8_t i;

        for (i = offset; i < ActiveConfiguration.UidSize; i++)
        {
            if (Carry)
            {
                if (UidBuffer[i] == 0x00)
                {
                    Carry = 1;
                }
                else
                {
                    Carry = 0;
                }

                UidBuffer[i] = (UidBuffer[i] - 1) & 0xFF;
            }
        }

        ApplicationSetUid(UidBuffer);
        break;
    }

    case BUTTON_ACTION_UID_RIGHT_DECREMENT:
    {
        ApplicationGetUid(UidBuffer);
        bool Carry = 1;
        uint8_t i = ActiveConfiguration.UidSize;

        while (i-- > 0)
        {
            if (Carry)
            {
                if (UidBuffer[i] == 0x00)
                {
                    Carry = 1;
                }
                else
                {
                    Carry = 0;
                }

                UidBuffer[i] = (UidBuffer[i] - 1) & 0xFF;
            }
        }

        ApplicationSetUid(UidBuffer);
        break;
    }

    case BUTTON_ACTION_CYCLE_SETTINGS:
    {
        SettingsCycle(true);
        break;
    }

    case BUTTON_ACTION_CYCLE_SETTINGS_DEC:
    {
        SettingsCycle(false);
        break;
    }

    case BUTTON_ACTION_STORE_MEM:
    {
        MemoryStore();
        break;
    }

    case BUTTON_ACTION_RECALL_MEM:
    {
        MemoryRecall();
        break;
    }

    case BUTTON_ACTION_TOGGLE_FIELD:
    {
        if (!CodecGetReaderField())
        {
            CodecReaderFieldStart();
        }
        else
        {
            CodecReaderFieldStop();
        }
        break;
    }

    case BUTTON_ACTION_STORE_LOG:
    {
        LogSRAMToFRAM();
        break;
    }

    case BUTTON_ACTION_CLEAR_LOG:
    {
        LogMemClear();
        DetectionLogClear();
        break;
    }

    case BUTTON_ACTION_CLONE:
    {
        CommandExecute("CLONE");
        break;
    }
    case BUTTON_ACTION_CLONE_MFU:
    {
        CommandExecute("CLONE_MFU");
        break;
    }

    default:
        break;
    }
}

void ButtonInit(void)
{
    BUTTON_PORT.DIRCLR = BUTTON_MASK;
    BUTTON_PORT.BUTTON_R_PINCTRL = PORT_OPC_PULLUP_gc;
    BUTTON_PORT.BUTTON_L_PINCTRL = PORT_OPC_PULLUP_gc;
}

void ButtonTick(void)
{
    static uint8_t ButtonRPressTick = 0;
    static uint8_t ButtonLPressTick = 0;
    uint8_t ThisButtonState = ~BUTTON_PORT.IN;

    if (ThisButtonState & BUTTON_R)
    {
        /* Button is currently pressed */
        if (ButtonRPressTick < LONG_PRESS_TICK_COUNT)
        {
            /* Count ticks while button is being pressed */
            ButtonRPressTick++;
        }
        else if (ButtonRPressTick == LONG_PRESS_TICK_COUNT)
        {
            /* Long button press detected execute button action and advance PressTickCounter
             * to an invalid state. */
            ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonActions[BUTTON_R_PRESS_LONG]);
            ButtonRPressTick++;
        }
        else
        {
            /* Button is still pressed, ignore */
        }
    }
    else if (!(ThisButtonState & BUTTON_MASK))
    {
        /* Button is currently not being pressed. Check if PressTickCounter contains
         * a recent short button press. */
        if ((ButtonRPressTick > 0) && (ButtonRPressTick <= LONG_PRESS_TICK_COUNT))
        {
            /* We have a short button press */
            ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonActions[BUTTON_R_PRESS_SHORT]);
        }

        ButtonRPressTick = 0;
    }

    if (ThisButtonState & BUTTON_L)
    {
        /* Button is currently pressed */
        if (ButtonLPressTick < LONG_PRESS_TICK_COUNT)
        {
            /* Count ticks while button is being pressed */
            ButtonLPressTick++;
        }
        else if (ButtonLPressTick == LONG_PRESS_TICK_COUNT)
        {
            /* Long button press detected execute button action and advance PressTickCounter
             * to an invalid state. */
            ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonActions[BUTTON_L_PRESS_LONG]);
            ButtonLPressTick++;
        }
        else
        {
            /* Button is still pressed, ignore */
        }
    }
    else if (!(ThisButtonState & BUTTON_MASK))
    {
        /* Button is currently not being pressed. Check if PressTickCounter contains
         * a recent short button press. */
        if ((ButtonLPressTick > 0) && (ButtonLPressTick <= LONG_PRESS_TICK_COUNT))
        {
            /* We have a short button press */
            ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonActions[BUTTON_L_PRESS_SHORT]);
        }

        ButtonLPressTick = 0;
    }
}

void ButtonGetActionList(char *List, uint16_t BufferSize)
{
    MapToString(ButtonActionMap, ARRAY_COUNT(ButtonActionMap), List, BufferSize);
}

void ButtonSetActionById(ButtonTypeEnum Type, ButtonActionEnum Action)
{
#ifndef BUTTON_SETTING_GLOBAL
    GlobalSettings.ActiveSettingPtr->ButtonActions[Type] = Action;
#else
    /* Write button action to all settings when using global settings */
    for (uint8_t i = 0; i < SETTINGS_COUNT; i++)
    {
        GlobalSettings.Settings[i].ButtonActions[Type] = Action;
    }
#endif
}

void ButtonGetActionByName(ButtonTypeEnum Type, char *Action, uint16_t BufferSize)
{
    MapIdToText(ButtonActionMap, ARRAY_COUNT(ButtonActionMap),
                GlobalSettings.ActiveSettingPtr->ButtonActions[Type], Action, BufferSize);
}

bool ButtonSetActionByName(ButtonTypeEnum Type, const char *Action)
{
    MapIdType Id;

    if (MapTextToId(ButtonActionMap, ARRAY_COUNT(ButtonActionMap), Action, &Id))
    {
        ButtonSetActionById(Type, Id);
        return true;
    }
    else
    {
        return false;
    }
}

