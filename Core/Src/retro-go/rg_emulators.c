#include <odroid_system.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "gw_linker.h"
#include "rg_emulators.h"
// #include "rg_favorites.h"
#include "bitmaps.h"
#include "gui.h"
#include "rom_manager.h"
#include "gw_lcd.h"
#include "main.h"
#include "main_gb.h"
#include "main_nes.h"
#include "main_smsplusgx.h"
#include "main_pce.h"
#include "main_gw.h"

// Increase when adding new emulators
#define MAX_EMULATORS 8
static retro_emulator_t emulators[MAX_EMULATORS];
static int emulators_count = 0;

retro_emulator_t *file_to_emu(retro_emulator_file_t *file) {
    for (int i = 0; i < MAX_EMULATORS; i++)
        if (emulators[i].system == file->system)
            return &emulators[i];
    return NULL;
}

static void event_handler(gui_event_t event, tab_t *tab)
{
    retro_emulator_t *emu = (retro_emulator_t *)tab->arg;
    listbox_item_t *item = gui_get_selected_item(tab);
    retro_emulator_file_t *file = (retro_emulator_file_t *)(item ? item->arg : NULL);

    if (event == TAB_INIT)
    {
        emulator_init(emu);

        if (emu->roms.count > 0)
        {
            sprintf(tab->status, " Games: %d", emu->roms.count);
            gui_resize_list(tab, emu->roms.count);

            for (int i = 0; i < emu->roms.count; i++)
            {
                tab->listbox.items[i].text = emu->roms.files[i].name;
                tab->listbox.items[i].arg = (void *)&emu->roms.files[i];
            }

            gui_sort_list(tab, 0);
            tab->is_empty = false;
        }
        else
        {
            sprintf(tab->status, " No games");
            gui_resize_list(tab, 8);
            size_t len = 0;
            tab->listbox.items[0].text = asnprintf(NULL, &len, "Place roms in folder: /roms/%s", emu->dirname);
            len = 0;
            tab->listbox.items[2].text = asnprintf(NULL, &len, "With file extension: .%s", emu->ext);
            tab->listbox.items[4].text = "Use SELECT and START to navigate.";
            tab->listbox.cursor = 3;
            tab->is_empty = true;
        }
    }

    /* The rest of the events require a file to be selected */
    if (file == NULL)
        return;

    if (event == KEY_PRESS_A)
    {
        emulator_show_file_menu(file);
        gui_redraw();
    }
    else if (event == KEY_PRESS_B)
    {
        emulator_show_file_info(file);
        gui_redraw();
    }
    else if (event == TAB_IDLE)
    {
        if (file->checksum == 0) {
            emulator_crc32_file(file);
        }

        // if (gui.show_cover && gui.idle_counter == (gui.show_cover == 1 ? 8 : 1)) {
        //     gui_draw_cover(file);
        // }
    }
    else if (event == TAB_REDRAW)
    {
        if (gui.show_cover)
            gui_draw_cover(file);
    }
}

static void add_emulator(const char *system, const char *dirname, const char* ext, const char *part,
                          uint16_t crc_offset, const void *header)
{
    assert(emulators_count <= MAX_EMULATORS);
    retro_emulator_t *p = &emulators[emulators_count++];
    strcpy(p->system_name, system);
    strcpy(p->dirname, dirname);
    strcpy(p->ext, ext);
    p->partition = 0;
    p->roms.count = 0;
    p->roms.files = NULL;
    p->initialized = false;
    p->crc_offset = crc_offset;

    gui_add_tab(dirname, header, p, event_handler);

    emulator_init(p);
}

void emulator_init(retro_emulator_t *emu)
{
    if (emu->initialized)
        return;

    emu->initialized = true;

    printf("Retro-Go: Initializing emulator '%s'\n", emu->system_name);


    const rom_system_t *system = rom_manager_system(&rom_mgr, emu->system_name);
    if (system) {
        emu->system = system;
        emu->roms.files = system->roms;
        emu->roms.count = system->roms_count;
    } else {
        while(1) {
            lcd_backlight_on();
            HAL_Delay(100);
            lcd_backlight_off();
            HAL_Delay(100);
        }
    }

    // retro_emulator_file_t *file = &emu->roms.files[emu->roms.count++];
    // strcpy(file->folder, "/");
    // strcpy(file->name, "test");
    // strcpy(file->ext, "gb");
    // file->emulator = (void*)emu;
    // file->crc_offset = emu->crc_offset;
    // file->checksum = 0;


    // char path[128];
    // char *files = NULL;
    // size_t count = 0;

    // sprintf(path, ODROID_BASE_PATH_CRC_CACHE "/%s", emu->dirname);
    // odroid_sdcard_mkdir(path);

    // sprintf(path, ODROID_BASE_PATH_SAVES "/%s", emu->dirname);
    // odroid_sdcard_mkdir(path);

    // sprintf(path, ODROID_BASE_PATH_ROMS "/%s", emu->dirname);
    // odroid_sdcard_mkdir(path);

    // if (odroid_sdcard_list(path, &files, &count) == 0 && count > 0)
    // {
    //     emu->roms.files = rg_alloc(count * sizeof(retro_emulator_file_t), MEM_ANY);
    //     emu->roms.count = 0;

    //     char *ptr = files;
    //     for (int i = 0; i < count; ++i)
    //     {
    //         const char *name = ptr;
    //         const char *ext = odroid_sdcard_get_extension(ptr);
    //         size_t name_len = strlen(name);

    //         // Advance pointer to next entry
    //         ptr += name_len + 1;

    //         if (!ext || strcasecmp(emu->ext, ext) != 0) //  && strcasecmp("zip", ext) != 0
    //             continue;

    //         retro_emulator_file_t *file = &emu->roms.files[emu->roms.count++];
    //         strcpy(file->folder, path);
    //         strcpy(file->name, name);
    //         strcpy(file->ext, ext);
    //         file->name[name_len-strlen(ext)-1] = 0;
    //         file->emulator = (void*)emu;
    //         file->crc_offset = emu->crc_offset;
    //         file->checksum = 0;
    //     }
    // }
    // free(files);
}

const uint32_t *emu_get_file_address(retro_emulator_file_t *file)
{
    // static char buffer[192];
    // if (file == NULL) return NULL;
    // sprintf(buffer, "%s/%s.%s", file->folder, file->name, file->ext);
    // return (const char*)&buffer;
    return (uint32_t *) file->address;
}

/*bool emulator_build_file_object(const char *path, retro_emulator_file_t *file)
{
    const char *name = odroid_sdcard_get_filename(path);
    const char *ext = odroid_sdcard_get_extension(path);

    if (ext == NULL || name == NULL)
        return false;

    memset(file, 0, sizeof(retro_emulator_file_t));
    strncpy(file->folder, path, strlen(path)-strlen(name)-1);
    strncpy(file->name, name, strlen(name)-strlen(ext)-1);
    strcpy(file->ext, ext);

    const char *dirname = odroid_sdcard_get_filename(file->folder);

    for (int i = 0; i < emulators_count; ++i)
    {
        if (strcmp(emulators[i].dirname, dirname) == 0)
        {
            file->crc_offset = emulators[i].crc_offset;
            file->emulator = &emulators[i];
            return true;
        }
    }

    return false;
}*/

void emulator_crc32_file(retro_emulator_file_t *file)
{
    // if (file == NULL || file->checksum > 0)
    //     return;

    // const int chunk_size = 32768;
    // const char *file_path = emu_get_file_path(file);
    // char *cache_path = odroid_system_get_path(ODROID_PATH_CRC_CACHE, file_path);
    // FILE *fp, *fp2;

    // file->missing_cover = 0;

    // if ((fp = fopen(cache_path, "rb")) != NULL)
    // {
    //     fread(&file->checksum, 4, 1, fp);
    //     fclose(fp);
    // }
    // else if ((fp = fopen(file_path, "rb")) != NULL)
    // {
    //     void *buffer = malloc(chunk_size);
    //     uint32_t crc_tmp = 0;
    //     uint32_t count = 0;

    //     gui_draw_notice("        CRC32", C_GREEN);

    //     fseek(fp, file->crc_offset, SEEK_SET);
    //     while (true)
    //     {
    //         odroid_input_read_gamepad(&gui.joystick);
    //         if (gui.joystick.bitmask > 0) break;

    //         count = fread(buffer, 1, chunk_size, fp);
    //         if (count == 0) break;

    //         crc_tmp = crc32_le(crc_tmp, buffer, count);
    //         if (count < chunk_size) break;
    //     }

    //     free(buffer);

    //     if (feof(fp))
    //     {
    //         file->checksum = crc_tmp;
    //         if ((fp2 = fopen(cache_path, "wb")) != NULL)
    //         {
    //             fwrite(&file->checksum, 4, 1, fp2);
    //             fclose(fp2);
    //         }
    //     }
    //     fclose(fp);
    // }
    // else
    // {
    //     file->checksum = 1;
    // }

    // free(cache_path);

    // gui_draw_notice(" ", C_RED);
}

void emulator_show_file_info(retro_emulator_file_t *file)
{
    char filename_value[128];
    char type_value[32];
    char size_value[32];
    char crc_value[32];
    crc_value[0] = '\x00';

    odroid_dialog_choice_t choices[] = {
        {0, "File", filename_value, 1, NULL},
        {0, "Type", type_value, 1, NULL},
        {0, "Folder", "...", 1, NULL},
        {0, "Size", size_value, 1, NULL},
        {0, "CRC32", crc_value, 1, NULL},
        {0, "---", "", -1, NULL},
        {1, "Close", "", 1, NULL},
        ODROID_DIALOG_CHOICE_LAST
    };

    sprintf(choices[0].value, "%.127s", file->name);
    sprintf(choices[1].value, "%s", file->ext);
    // sprintf(choices[2].value, "%s", file->folder);
    // sprintf(choices[3].value, "%d KB", odroid_sdcard_get_filesize(emu_get_file_path(file)) / 1024);
    sprintf(choices[3].value, "%d KB", file->size / 1024);

    if (file->checksum > 1)
    {
        if (file->crc_offset)
            sprintf(choices[4].value, "%08lX (%d)", file->checksum, file->crc_offset);
        else
            sprintf(choices[4].value, "%08lX", file->checksum);
    }

    odroid_overlay_dialog("Properties", choices, -1);
}

void emulator_show_file_menu(retro_emulator_file_t *file)
{
    // char *save_path = odroid_system_get_path(ODROID_PATH_SAVE_STATE, emu_get_file_path(file));
    // char *sram_path = odroid_system_get_path(ODROID_PATH_SAVE_SRAM, emu_get_file_path(file));
    // bool has_save = odroid_sdcard_get_filesize(save_path) > 0;
    // bool has_sram = odroid_sdcard_get_filesize(sram_path) > 0;

    bool has_save = 1;
    bool has_sram = 0;

    odroid_dialog_choice_t choices[] = {
#if STATE_SAVING == 1
        {0, "Resume game ", "", has_save, NULL},
#endif
        {1, "New game    ", "", 1, NULL},
        {0, "------------", "", -1, NULL},
#if STATE_SAVING == 1
        {2, "Delete save ", "", has_save || has_sram, NULL},
#endif
        ODROID_DIALOG_CHOICE_LAST
    };
    int sel = odroid_overlay_dialog(NULL, choices, has_save ? 0 : 1);

    if (sel == 0 || sel == 1) {
        gui_save_current_tab();
        emulator_start(file, sel == 0, false);
    }
    else if (sel == 2) {
        if (odroid_overlay_confirm("Delete save file?", false) == 1) {
            store_erase(file->save_address, file->save_size);
        }
    }

    // free(save_path);
    // free(sram_path);
}

void emulator_start(retro_emulator_file_t *file, bool load_state, bool start_paused)
{
    printf("Retro-Go: Starting game: %s\n", file->name);
    rom_manager_set_active_file(file);

    // odroid_settings_StartAction_set(load_state ? ODROID_START_ACTION_RESUME : ODROID_START_ACTION_NEWGAME);
    // odroid_settings_RomFilePath_set(path);
    // odroid_settings_commit();

    // odroid_system_switch_app(((retro_emulator_t *)file->emulator)->partition);
    retro_emulator_t *emu = file_to_emu(file);
    // TODO: Make this cleaner
    if(strcmp(emu->system_name, "Nintendo Gameboy") == 0) {
#ifdef ENABLE_EMULATOR_GB
        memcpy(&__RAM_EMU_START__, &_OVERLAY_GB_LOAD_START, (size_t)&_OVERLAY_GB_SIZE);
        memset(&_OVERLAY_GB_BSS_START, 0x0, (size_t)&_OVERLAY_GB_BSS_SIZE);
        SCB_CleanDCache_by_Addr((uint32_t *)&__RAM_EMU_START__, (size_t)&_OVERLAY_GB_SIZE);
        app_main_gb(load_state, start_paused);
#endif
    } else if(strcmp(emu->system_name, "Nintendo Entertainment System") == 0) {
#ifdef ENABLE_EMULATOR_NES
        memcpy(&__RAM_EMU_START__, &_OVERLAY_NES_LOAD_START, (size_t)&_OVERLAY_NES_SIZE);
        memset(&_OVERLAY_NES_BSS_START, 0x0, (size_t)&_OVERLAY_NES_BSS_SIZE);
        SCB_CleanDCache_by_Addr((uint32_t *)&__RAM_EMU_START__, (size_t)&_OVERLAY_NES_SIZE);
        app_main_nes(load_state, start_paused);
#endif
    } else if(strcmp(emu->system_name, "Sega Master System") == 0 ||
              strcmp(emu->system_name, "Sega Game Gear") == 0     ||
              strcmp(emu->system_name, "Sega SG-1000") == 0       ||
              strcmp(emu->system_name, "Colecovision") == 0 ) {
#if defined(ENABLE_EMULATOR_SMS) || defined(ENABLE_EMULATOR_GG) || defined(ENABLE_EMULATOR_COL) || defined(ENABLE_EMULATOR_SG1000)
        memcpy(&__RAM_EMU_START__, &_OVERLAY_SMS_LOAD_START, (size_t)&_OVERLAY_SMS_SIZE);
        memset(&_OVERLAY_SMS_BSS_START, 0x0, (size_t)&_OVERLAY_SMS_BSS_SIZE);
        SCB_CleanDCache_by_Addr((uint32_t *)&__RAM_EMU_START__, (size_t)&_OVERLAY_SMS_SIZE);
        if (! strcmp(emu->system_name, "Colecovision")) app_main_smsplusgx(load_state, start_paused, SMSPLUSGX_ENGINE_COLECO);
        else
        if (! strcmp(emu->system_name, "Sega SG-1000")) app_main_smsplusgx(load_state, start_paused, SMSPLUSGX_ENGINE_SG1000);
        else                                            app_main_smsplusgx(load_state, start_paused, SMSPLUSGX_ENGINE_OTHERS);
#endif
    } else if(strcmp(emu->system_name, "Game & Watch") == 0 ) {
#ifdef ENABLE_EMULATOR_GW
        memcpy(&__RAM_EMU_START__, &_OVERLAY_GW_LOAD_START, (size_t)&_OVERLAY_GW_SIZE);
        memset(&_OVERLAY_GW_BSS_START, 0x0, (size_t)&_OVERLAY_GW_BSS_SIZE);
        SCB_CleanDCache_by_Addr((uint32_t *)&__RAM_EMU_START__, (size_t)&_OVERLAY_GW_SIZE);
        app_main_gw(load_state);
#endif
    } else if(strcmp(emu->system_name, "PC Engine") == 0) {
#ifdef ENABLE_EMULATOR_PCE
      memcpy(&__RAM_EMU_START__, &_OVERLAY_PCE_LOAD_START, (size_t)&_OVERLAY_PCE_SIZE);
      memset(&_OVERLAY_PCE_BSS_START, 0x0, (size_t)&_OVERLAY_PCE_BSS_SIZE);
      SCB_CleanDCache_by_Addr((uint32_t *)&__RAM_EMU_START__, (size_t)&_OVERLAY_PCE_SIZE);
      app_main_pce(load_state, start_paused);
#endif
  }
    
}

void emulators_init()
{
#if !( defined(ENABLE_EMULATOR_GB) || defined(ENABLE_EMULATOR_NES) || defined(ENABLE_EMULATOR_SMS) || defined(ENABLE_EMULATOR_GG) || defined(ENABLE_EMULATOR_COL) || defined(ENABLE_EMULATOR_SG1000) || defined(ENABLE_EMULATOR_PCE) || defined(ENABLE_EMULATOR_GW))
    // Add gameboy as a placeholder in case no emulator is built.
    add_emulator("Nintendo Gameboy", "gb", "gb", "gnuboy-go", 0, header_gb);
#endif

#ifdef ENABLE_EMULATOR_GB
    add_emulator("Nintendo Gameboy", "gb", "gb", "gnuboy-go", 0, header_gb);
#endif

#ifdef ENABLE_EMULATOR_NES
    add_emulator("Nintendo Entertainment System", "nes", "nes", "nofrendo-go", 16, header_nes);
#endif
    
#ifdef ENABLE_EMULATOR_SMS
    add_emulator("Sega Master System", "sms", "sms", "smsplusgx-go", 0, header_sms);
#endif

#ifdef ENABLE_EMULATOR_GG
    add_emulator("Sega Game Gear", "gg", "gg", "smsplusgx-go", 0, header_gg);
#endif

#ifdef ENABLE_EMULATOR_COL
    add_emulator("Colecovision", "col", "col", "smsplusgx-go", 0, header_col);
#endif

#ifdef ENABLE_EMULATOR_SG1000
    add_emulator("Sega SG-1000", "sg", "sg", "smsplusgx-go", 0, header_sg1000);
#endif

#ifdef ENABLE_EMULATOR_PCE
    add_emulator("PC Engine", "pce", "pce", "huexpress-go", 0, header_pce);
#endif

#ifdef ENABLE_EMULATOR_GW
    add_emulator("Game & Watch", "gw", "gw", "LCD-Game-Emulator", 0, header_gw);
#endif

}

bool emulator_is_file_valid(retro_emulator_file_t *file)
{
    for (int i = 0; i < emulators_count; i++) {
        for (int j = 0; j < emulators[i].roms.count; j++) {
            if (&emulators[i].roms.files[j] == file) {
                return true;
            }
        }
    }

    return false;
}
