#include "wow.h"

#include "itf/interface.h"

#include "ppe/render_target.h"
#include "ppe/render_pass.h"

#include "net/network.h"
#include "net/packet.h"
#include "net/opcode.h"

#include "game/social.h"
#include "game/group.h"

#include "font/font.h"

#include "obj/update_fields.h"
#include "obj/player.h"

#include "gx/frame.h"
#include "gx/m2.h"

#include "snd/snd.h"

#include "map/map.h"

#include "performance.h"
#include "lagometer.h"
#include "graphics.h"
#include "shaders.h"
#include "wow_lua.h"
#include "camera.h"
#include "loader.h"
#include "memory.h"
#include "const.h"
#include "cache.h"
#include "cvars.h"
#include "log.h"
#include "dbc.h"
#include "db.h"

#include <libwow/mpq.h>
#include <libwow/trs.h>

#include <gfx/window.h>
#include <gfx/device.h>

#include <jks/array.h>
#include <jks/hmap.h>

#include <ft2build.h>
#include FT_MODULE_H
#include FT_SYSTEM_H

#include <inttypes.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <png.h>

#ifdef _WIN32
# include <windows.h>
double performance_frequency;
#endif

#ifdef interface
# undef interface
#endif

MEMORY_DECL(GENERIC);
MEMORY_DECL(LIBWOW);
MEMORY_DECL(GFX);

struct wow *g_wow = NULL;

static void resize_callback(struct gfx_resize_event *event);
static void key_down_callback(struct gfx_key_event *event);
static void key_up_callback(struct gfx_key_event *event);
static void key_press_callback(struct gfx_key_event *event);
static void char_callback(struct gfx_char_event *event);
static void mouse_down_callback(struct gfx_mouse_event *event);
static void mouse_up_callback(struct gfx_mouse_event *event);
static void mouse_move_callback(struct gfx_pointer_event *event);
static void mouse_scroll_callback(struct gfx_scroll_event *event);
static void error_callback(const char *fmt, ...);

static struct gfx_window *wow_create_window(const char *title)
{
	LOG_INFO("creating window");
	struct gfx_window_properties properties;
	gfx_window_properties_init(&properties);
	properties.window_backend = (enum gfx_window_backend)g_wow->window_backend;
	properties.device_backend = (enum gfx_device_backend)g_wow->device_backend;
	properties.depth_bits = 24;
	properties.stencil_bits = 8;
	properties.red_bits = 8;
	properties.green_bits = 8;
	properties.blue_bits = 8;
	properties.alpha_bits = 8;
	return gfx_create_window(title, g_wow->render_width, g_wow->render_height, &properties);
}

static bool setup_icon(void)
{
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_infop end_info = NULL;
	png_bytep *row_pointers = NULL;
	png_byte header[8];
	png_byte *image_data = NULL;
	FILE *fp = NULL;
	int rowbytes;
	uint32_t width;
	uint32_t height;
	if (!(fp = fopen("icon.png", "rb")))
		goto error1;
	if (fread(header, 1, 8, fp) != 8)
		goto error2;
	if (png_sig_cmp(header, 0, 8))
		goto error2;
	if (!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		goto error2;
	if (!(info_ptr = png_create_info_struct(png_ptr)))
		goto error3;
	if (!(end_info = png_create_info_struct(png_ptr)))
		goto error3;
	if (setjmp(png_jmpbuf(png_ptr)))
		goto error3;
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	int bit_depth, color_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	png_set_palette_to_rgb(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	image_data = mem_malloc(MEM_GENERIC, rowbytes * height);
	if (!image_data)
		goto error3;
	row_pointers = mem_malloc(MEM_GENERIC, sizeof(png_bytep) * height);
	if (!row_pointers)
		goto error4;
	for (uint32_t i = 0; i < height; ++i)
		row_pointers[i] = image_data + i * rowbytes;
	png_read_image(png_ptr, row_pointers);
	gfx_window_set_icon(g_wow->window, image_data, width, height);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	mem_free(MEM_GENERIC, image_data);
	mem_free(MEM_GENERIC, row_pointers);
	fclose(fp);
	return true;
error4:
	mem_free(MEM_GENERIC, image_data);
error3:
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
error2:
	fclose(fp);
error1:
	return false;
}

static bool setup_window(enum gfx_window_backend window_backend, enum gfx_device_backend device_backend)
{
	LOG_INFO("creating main window");
	g_wow->window_backend = window_backend;
	g_wow->device_backend = device_backend;
	g_wow->render_width = 1370;
	g_wow->render_height = 760;
	g_wow->window = wow_create_window("WoW");
	if (!g_wow->window)
	{
		LOG_ERROR("no window created");
		return false;
	}
	g_wow->window->resize_callback = resize_callback;
	g_wow->window->key_down_callback = key_down_callback;
	g_wow->window->key_up_callback = key_up_callback;
	g_wow->window->key_press_callback = key_press_callback;
	g_wow->window->char_callback = char_callback;
	g_wow->window->mouse_down_callback = mouse_down_callback;
	g_wow->window->mouse_up_callback = mouse_up_callback;
	g_wow->window->mouse_move_callback = mouse_move_callback;
	g_wow->window->scroll_callback = mouse_scroll_callback;
	gfx_window_make_current(g_wow->window);
	LOG_INFO("displaying window");
	gfx_window_show(g_wow->window);
	if (!gfx_create_device(g_wow->window))
	{
		LOG_ERROR("failed to create device");
		return false;
	}
	g_wow->device = g_wow->window->device;
	gfx_window_set_swap_interval(g_wow->window, -1);
	g_wow->wow_opt |= WOW_OPT_VSYNC;
	setup_icon();
	return true;
}

static bool setup_graphics(void)
{
	g_wow->graphics = mem_malloc(MEM_GENERIC, sizeof(*g_wow->graphics));
	if (!g_wow->graphics)
		return false;
	if (!graphics_init(g_wow->graphics))
		return false;
	return true;
}

static bool setup_shaders(void)
{
	g_wow->shaders = mem_malloc(MEM_GENERIC, sizeof(*g_wow->shaders));
	if (!g_wow->shaders)
		return false;
	if (!shaders_build(g_wow->shaders))
		return false;
	return true;
}

static bool setup_map(void)
{
	LOG_INFO("loading map");
	g_wow->map = map_new();
	if (!g_wow->map)
		return false;
	return true;
}

bool wow_set_map(uint32_t mapid)
{
	if (!g_wow->map)
		return true;
	if (!map_setid(g_wow->map, mapid))
		return false;
	return true;
}

static bool setup_cache(void)
{
	g_wow->cache = cache_new();
	if (!g_wow->cache)
		return false;
	return true;
}

static void archive_delete(void *ptr)
{
	wow_mpq_archive_delete(*(struct wow_mpq_archive**)ptr);
}

static bool setup_game_files(void)
{
	g_wow->mpq_archives = mem_malloc(MEM_GENERIC, sizeof(*g_wow->mpq_archives));
	if (!g_wow->mpq_archives)
	{
		LOG_ERROR("malloc failed");
		return EXIT_FAILURE;
	}
	jks_array_init(g_wow->mpq_archives, sizeof(struct wow_mpq_archive*), archive_delete, &jks_array_memory_fn_GENERIC);
	char files[14][256];
	snprintf(files[0] , sizeof(files[0]) , "patch-5.MPQ");
	snprintf(files[1] , sizeof(files[1]) , "patch-3.MPQ");
	snprintf(files[2] , sizeof(files[2]) , "patch-2.MPQ");
	snprintf(files[3] , sizeof(files[3]) , "patch.MPQ");
	snprintf(files[4] , sizeof(files[4]) , "%s/patch-%s-2.MPQ", g_wow->locale, g_wow->locale);
	snprintf(files[5] , sizeof(files[5]) , "%s/patch-%s.MPQ", g_wow->locale, g_wow->locale);
	snprintf(files[6] , sizeof(files[6]) , "expansion.MPQ");
	snprintf(files[7] , sizeof(files[7]) , "common.MPQ");
	snprintf(files[8] , sizeof(files[8]) , "%s/base-%s.MPQ", g_wow->locale, g_wow->locale);
	snprintf(files[9] , sizeof(files[9]) , "%s/backup-%s.MPQ", g_wow->locale, g_wow->locale);
	snprintf(files[10], sizeof(files[10]), "%s/expansion-locale-%s.MPQ", g_wow->locale, g_wow->locale);
	snprintf(files[11], sizeof(files[11]), "%s/locale-%s.MPQ", g_wow->locale, g_wow->locale);
	snprintf(files[12], sizeof(files[12]), "%s/expansion-speech-%s.MPQ", g_wow->locale, g_wow->locale);
	snprintf(files[13], sizeof(files[13]), "%s/speech-%s.MPQ", g_wow->locale, g_wow->locale);
	/*char files[8][256];
	snprintf(files[0], sizeof(files[0]), "dbc.MPQ");
	snprintf(files[1], sizeof(files[1]), "fonts.MPQ");
	snprintf(files[2], sizeof(files[2]), "interface.MPQ");
	snprintf(files[3], sizeof(files[3]), "misc.MPQ");
	snprintf(files[4], sizeof(files[4]), "model.MPQ");
	snprintf(files[5], sizeof(files[5]), "sound.MPQ");
	snprintf(files[6], sizeof(files[6]), "speech.MPQ");
	snprintf(files[7], sizeof(files[7]), "texture.MPQ");*/
	for (size_t i = 0; i < sizeof(files) / sizeof(*files); ++i)
	{
		char name[512];
		snprintf(name, sizeof(name), "%s/Data/%s", g_wow->game_path, files[i]);
		struct wow_mpq_archive *archive = wow_mpq_archive_new(name);
		if (!archive)
		{
			LOG_ERROR("failed to open archive \"%s\"", name);
			continue;
		}
		if (!jks_array_push_back(g_wow->mpq_archives, &archive))
		{
			LOG_ERROR("failed to add archive to list");
			continue;
		}
	}
	LOG_INFO("loading MPQs");
	g_wow->mpq_compound = wow_mpq_compound_new();
	if (!g_wow->mpq_compound)
	{
		LOG_ERROR("failed to get compound");
		return false;
	}
	if (!wow_load_compound(g_wow->mpq_compound))
	{
		wow_mpq_compound_delete(g_wow->mpq_compound);
		g_wow->mpq_compound = NULL;
		return false;
	}
	return true;
}

static bool setup_interface(void)
{
	g_wow->interface = interface_new();
	if (!g_wow->interface)
		return false;
	{
		struct gfx_resize_event event;
		event.width = g_wow->window->width;
		event.height = g_wow->window->height;
		interface_on_window_resized(g_wow->interface, &event);
	}
	return true;
}

static bool setup_network(void)
{
	g_wow->network = network_new();
	if (!g_wow->network)
		return false;
	return true;
}

static bool setup_render_passes(void)
{
	LOG_INFO("loading MSAA render target");
	g_wow->post_process.msaa = render_target_new(8);
	if (!g_wow->post_process.msaa)
	{
		LOG_ERROR("failed to load MSAA render target");
		return false;
	}
	LOG_INFO("loading dummy render target");
	g_wow->post_process.dummy1 = render_target_new(0);
	if (!g_wow->post_process.dummy1)
	{
		LOG_ERROR("failed to load dummy render target 1");
		return false;
	}
	g_wow->post_process.dummy2 = render_target_new(0);
	if (!g_wow->post_process.dummy2)
	{
		LOG_ERROR("failed to load dummy render target 2");
		return false;
	}
	LOG_INFO("loading chromaber render pass");
	g_wow->post_process.chromaber = chromaber_render_pass_new();
	if (!g_wow->post_process.chromaber)
	{
		LOG_ERROR("failed to load chromaber render pass");
		return false;
	}
#if 0
	g_wow->post_process.chromaber->enabled = true;
#endif
	LOG_INFO("loading sharpen render pass");
	g_wow->post_process.sharpen = sharpen_render_pass_new();
	if (!g_wow->post_process.sharpen)
	{
		LOG_ERROR("failed to load sharpen render pass");
		return false;
	}
#if 0
	g_wow->post_process.sharpen->enabled = true;
#endif
	LOG_INFO("loading glow render pass");
	g_wow->post_process.glow = glow_render_pass_new();
	if (!g_wow->post_process.glow)
	{
		LOG_ERROR("failed to load glow render pass");
		return false;
	}
#if 0
	g_wow->post_process.glow->enabled = true;
#endif
	LOG_INFO("loading sobel render pass");
	g_wow->post_process.sobel = sobel_render_pass_new();
	if (!g_wow->post_process.sobel)
	{
		LOG_ERROR("failed to load sobel render pass");
		return false;
	}
#if 0
	g_wow->post_process.ssao->enabled = true;
#endif
	LOG_INFO("loading SSAO render pass");
	g_wow->post_process.ssao = ssao_render_pass_new();
	if (!g_wow->post_process.ssao)
	{
		LOG_ERROR("failed to load SSAO render pass");
		return false;
	}
#if 0
	g_wow->post_process.ssao->enabled = true;
#endif
	LOG_INFO("loading FXAA render pass");
	g_wow->post_process.fxaa = fxaa_render_pass_new();
	if (!g_wow->post_process.fxaa)
	{
		LOG_ERROR("failed to load FXAA render pass");
		return false;
	}
#if 0
	g_wow->post_process.fxaa->enabled = true;
#endif
	LOG_INFO("loading FSAA render pass");
	g_wow->post_process.fsaa = fsaa_render_pass_new();
	if (!g_wow->post_process.fsaa)
	{
		LOG_ERROR("failed to load FSAA render pass");
		return false;
	}
#if 0
	g_wow->post_process.fsaa->enabled = true;
#endif
	LOG_INFO("loading cel render pass");
	g_wow->post_process.cel = cel_render_pass_new();
	if (!g_wow->post_process.cel)
	{
		LOG_ERROR("failed to load cel render pass");
		return false;
	}
#if 0
	g_wow->post_process.cel->enabled = true;
#endif
	LOG_INFO("loading bloom render pass");
	g_wow->post_process.bloom = bloom_render_pass_new();
	if (!g_wow->post_process.bloom)
	{
		LOG_ERROR("failed to load bloom render pass");
		return false;
	}
#if 0
	g_wow->post_process.bloom->enabled = true;
#endif
	return true;
}

static void dirty_render_passes(void)
{
	if (g_wow->post_process.ssao)
		g_wow->post_process.ssao->dirty_size = true;
	if (g_wow->post_process.bloom)
		g_wow->post_process.bloom->dirty_size = true;
	if (g_wow->post_process.msaa)
		g_wow->post_process.msaa->dirty_size = true;
	if (g_wow->post_process.dummy1)
		g_wow->post_process.dummy1->dirty_size = true;
	if (g_wow->post_process.dummy2)
		g_wow->post_process.dummy2->dirty_size = true;
}

static bool load_dbc(void)
{
#define DBC_LOAD(name, var) \
	if (!cache_ref_by_key_dbc(g_wow->cache, "DBFilesClient\\" name, &g_wow->dbc.var)) \
	{ \
		LOG_ERROR("can't find %s", name); \
		return false; \
	}

#define DBC_LOAD_INDEXED(name, var, index, str_index) \
	do \
	{ \
		DBC_LOAD(name, var); \
		if (!dbc_set_index(g_wow->dbc.var, index, str_index)) \
		{ \
			LOG_ERROR("failed to set index %d to %s", index, name); \
			return false; \
		} \
	} while (0)

	DBC_LOAD_INDEXED("AreaPOI.dbc", area_poi, 0, false);
	DBC_LOAD_INDEXED("AreaTable.dbc", area_table, 0, false);
	DBC_LOAD_INDEXED("AuctionHouse.dbc", auction_house, 0, false);
	DBC_LOAD("CharBaseInfo.dbc", char_base_info);
	DBC_LOAD_INDEXED("CharHairGeosets.dbc", char_hair_geosets, 0, false);
	DBC_LOAD("CharSections.dbc", char_sections);
	DBC_LOAD_INDEXED("CharStartOutfit.dbc", char_start_outfit, 0, false);
	DBC_LOAD("CharacterFacialHairStyles.dbc", character_facial_hair_styles);
	DBC_LOAD_INDEXED("ChrRaces.dbc", chr_races, 0, false);
	DBC_LOAD_INDEXED("ChrClasses.dbc", chr_classes, 0, false);
	DBC_LOAD_INDEXED("CreatureDisplayInfo.dbc", creature_display_info, 0, false);
	DBC_LOAD_INDEXED("CreatureDisplayInfoExtra.dbc", creature_display_info_extra, 0, false);
	DBC_LOAD_INDEXED("CreatureModelData.dbc", creature_model_data, 0, false);
	DBC_LOAD_INDEXED("GameObjectDisplayInfo.dbc", game_object_display_info, 0, false);
	DBC_LOAD_INDEXED("GroundEffectTexture.dbc", ground_effect_texture, 0, false);
	DBC_LOAD_INDEXED("GroundEffectDoodad.dbc", ground_effect_doodad, 4, false);
	DBC_LOAD_INDEXED("HelmetGeosetVisData.dbc", helmet_geoset_vis_data, 0, false);
	DBC_LOAD_INDEXED("Item.dbc", item, 0, false);
	DBC_LOAD_INDEXED("ItemClass.dbc", item_class, 0, false);
	DBC_LOAD_INDEXED("ItemDisplayInfo.dbc", item_display_info, 0, false);
	DBC_LOAD_INDEXED("ItemSubClass.dbc", item_sub_class, 0, false);
	DBC_LOAD_INDEXED("Map.dbc", map, 0, false);
	DBC_LOAD_INDEXED("NameGen.dbc", name_gen, 0, false);
	DBC_LOAD_INDEXED("SoundEntries.dbc", sound_entries, 8, true);
	DBC_LOAD_INDEXED("Spell.dbc", spell, 0, false);
	DBC_LOAD_INDEXED("SpellIcon.dbc", spell_icon, 0, false);
	DBC_LOAD_INDEXED("Talent.dbc", talent, 0, false);
	DBC_LOAD_INDEXED("TalentTab.dbc", talent_tab, 0, false);
	DBC_LOAD_INDEXED("TaxiNodes.dbc", taxi_nodes, 0, false);
	DBC_LOAD_INDEXED("TaxiPath.dbc", taxi_path, 0, false);
	DBC_LOAD_INDEXED("WorldMapArea.dbc", world_map_area, 0, false);
	DBC_LOAD_INDEXED("WorldMapContinent.dbc", world_map_continent, 0, false);
	DBC_LOAD("WorldMapOverlay.dbc", world_map_overlay);
	return true;

#undef DBC_LOAD
#undef DBC_LOAD_INDEXED
}

static void unload_dbc(void)
{
#define DBC_UNLOAD(name) \
	do \
	{ \
		cache_unref_by_ref_dbc(g_wow->cache, g_wow->dbc.name); \
	} while (0)

	DBC_UNLOAD(area_poi);
	DBC_UNLOAD(area_table);
	DBC_UNLOAD(auction_house);
	DBC_UNLOAD(char_base_info);
	DBC_UNLOAD(char_sections);
	DBC_UNLOAD(char_hair_geosets);
	DBC_UNLOAD(char_start_outfit);
	DBC_UNLOAD(character_facial_hair_styles);
	DBC_UNLOAD(chr_races);
	DBC_UNLOAD(chr_classes);
	DBC_UNLOAD(creature_display_info);
	DBC_UNLOAD(creature_model_data);
	DBC_UNLOAD(creature_display_info_extra);
	DBC_UNLOAD(game_object_display_info);
	DBC_UNLOAD(ground_effect_texture);
	DBC_UNLOAD(ground_effect_doodad);
	DBC_UNLOAD(helmet_geoset_vis_data);
	DBC_UNLOAD(item);
	DBC_UNLOAD(item_class);
	DBC_UNLOAD(item_display_info);
	DBC_UNLOAD(item_sub_class);
	DBC_UNLOAD(map);
	DBC_UNLOAD(name_gen);
	DBC_UNLOAD(sound_entries);
	DBC_UNLOAD(spell);
	DBC_UNLOAD(spell_icon);
	DBC_UNLOAD(talent);
	DBC_UNLOAD(talent_tab);
	DBC_UNLOAD(taxi_nodes);
	DBC_UNLOAD(taxi_path);
	DBC_UNLOAD(world_map_area);
	DBC_UNLOAD(world_map_continent);
	DBC_UNLOAD(world_map_overlay);

#undef DBC_UNLOAD
}

static void setup_gui(void)
{
	LOG_INFO("loading GUI");
	g_wow->lagometer = lagometer_new();
}

static void trs_entry_dtr(jks_hmap_key_t key, void *value)
{
	mem_free(MEM_GENERIC, key.ptr);
	mem_free(MEM_GENERIC, *(char**)value);
}

static void trs_dir_dtr(jks_hmap_key_t key, void *value)
{
	mem_free(MEM_GENERIC, key.ptr);
	struct trs_dir *dir = value;
	if (!dir->entries)
		return;
	jks_hmap_destroy(dir->entries);
	free(dir->entries);
}

static bool load_trs(void)
{
	/* XXX should load only current map entries */
	struct wow_mpq_file *mpq = wow_mpq_get_file(g_wow->mpq_compound, "TEXTURES\\MINIMAP\\MD5TRANSLATE.TRS");
	if (!mpq)
	{
		LOG_ERROR("failed to get TRS mpq file");
		return false;
	}
	struct wow_trs_file *trs = wow_trs_file_new(mpq);
	if (!trs)
	{
		LOG_ERROR("failed to parse TRS file");
		return false;
	}
	g_wow->trs = mem_malloc(MEM_GENERIC, sizeof(*g_wow->trs));
	if (!g_wow->trs)
	{
		LOG_ERROR("allocation failed");
		return false;
	}
	jks_hmap_init(g_wow->trs, sizeof(struct trs_dir), trs_dir_dtr, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_GENERIC);
	for (size_t i = 0; i < trs->dirs_nb; ++i)
	{
		struct trs_dir dir;
		char keyname[256];
		snprintf(keyname, sizeof(keyname), "%s", trs->dirs[i].name);
		normalize_mpq_filename(keyname, sizeof(keyname));
		char *key = mem_strdup(MEM_GENERIC, keyname);
		if (!key)
		{
			LOG_ERROR("allocation failed");
			goto err;
		}
		dir.entries = mem_malloc(MEM_GENERIC, sizeof(*dir.entries));
		if (!dir.entries)
		{
			LOG_ERROR("allocation failed");
			mem_free(MEM_GENERIC, key);
			goto err;
		}
		jks_hmap_init(dir.entries, sizeof(char*), trs_entry_dtr, jks_hmap_hash_str, jks_hmap_cmp_str, &jks_hmap_memory_fn_GENERIC);
		for (size_t j = 0; j < trs->dirs[i].entries_nb; ++j)
		{
			char name[256];
			char hash[256];
			snprintf(name, sizeof(name), "%s", trs->dirs[i].entries[j].name);
			snprintf(hash, sizeof(hash), "%s", trs->dirs[i].entries[j].hash);
			normalize_mpq_filename(name, sizeof(name));
			normalize_mpq_filename(hash, sizeof(hash));
			char *namedup = mem_strdup(MEM_GENERIC, name);
			char *hashdup = mem_strdup(MEM_GENERIC, hash);
			if (!namedup || !hashdup)
			{
				LOG_ERROR("allocation failed");
				mem_free(MEM_GENERIC, namedup);
				mem_free(MEM_GENERIC, hashdup);
				jks_hmap_destroy(dir.entries);
				mem_free(MEM_GENERIC, dir.entries);
				goto err;
			}
			if (!jks_hmap_set(dir.entries, JKS_HMAP_KEY_STR(namedup), &hashdup))
			{
				LOG_ERROR("failed to set trs entry");
				mem_free(MEM_GENERIC, namedup);
				mem_free(MEM_GENERIC, hashdup);
				jks_hmap_destroy(dir.entries);
				mem_free(MEM_GENERIC, dir.entries);
				goto err;
			}
		}
		if (!jks_hmap_set(g_wow->trs, JKS_HMAP_KEY_STR((char*)key), &dir))
		{
			LOG_ERROR("failed to set trs dir");
			mem_free(MEM_GENERIC, key);
			goto err;
		}
	}
	return true;
err:
	jks_hmap_destroy(g_wow->trs);
	mem_free(MEM_GENERIC, g_wow->trs);
	return false;
}

static void loop(void)
{
	gfx_depth_stencil_state_t depth_stencil_state = GFX_DEPTH_STENCIL_STATE_INIT();
	gfx_rasterizer_state_t rasterizer_state = GFX_RASTERIZER_STATE_INIT();
	gfx_pipeline_state_t pipeline_state = GFX_PIPELINE_STATE_INIT();
	gfx_input_layout_t input_layout = GFX_INPUT_LAYOUT_INIT();
	gfx_blend_state_t blend_state = GFX_BLEND_STATE_INIT();

	gfx_create_depth_stencil_state(g_wow->device, &depth_stencil_state, true, true, GFX_CMP_LEQUAL, true, -1, GFX_CMP_NOTEQUAL, 1, -1, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP, GFX_STENCIL_KEEP);
	gfx_create_rasterizer_state(g_wow->device, &rasterizer_state, GFX_FILL_SOLID, GFX_CULL_NONE, GFX_FRONT_CCW, false);
	gfx_create_blend_state(g_wow->device, &blend_state, true, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE_MINUS_SRC_ALPHA, GFX_EQUATION_ADD, GFX_EQUATION_ADD, GFX_COLOR_MASK_ALL);
	gfx_create_input_layout(g_wow->device, &input_layout, NULL, 0, &g_wow->shaders->gui);
	gfx_create_pipeline_state(g_wow->device, &pipeline_state, &g_wow->shaders->gui, &rasterizer_state, &depth_stencil_state, &blend_state, &input_layout, GFX_PRIMITIVE_TRIANGLES);
	int64_t fps = 0;
	int64_t last_fps = nanotime();
	while (!g_wow->window->close_requested)
	{
#if 0
		LOG_INFO("x: %f, y: %f, z: %f", g_wow->view_camera->pos.x, g_wow->view_camera->pos.y, g_wow->view_camera->pos.z);
#endif
		bool culled_async = g_wow->wow_opt & WOW_OPT_ASYNC_CULL;
		uint64_t started, ended;
		started = nanotime();
		g_wow->lastframetime = g_wow->frametime;
		g_wow->frametime = nanotime();
		if (g_wow->frametime - last_fps >= 1000000000)
		{
#ifdef WITH_MEMORY
			mem_dump();
#endif
#ifdef WITH_PERFORMANCE
			performance_dump();
			performance_reset();
#endif
			last_fps = nanotime();
			char title[64];
			snprintf(title, sizeof(title), "%" PRId64 "fps", fps);
			gfx_window_set_title(g_wow->window, title);
			fps = 0;
		}
		g_wow->draw_frame_id = wow_frame_id(0);
		g_wow->cull_frame_id = wow_frame_id(1);
		g_wow->gc_frame_id = wow_frame_id(-1);
		g_wow->draw_frame = wow_frame(0);
		g_wow->cull_frame = wow_frame(1);
		g_wow->gc_frame = wow_frame(-1);
		camera_handle_keyboard(g_wow->view_camera);
		camera_handle_mouse(g_wow->view_camera);
		render_clear_scene(g_wow->cull_frame);
		if (!camera_update_matrixes(g_wow->view_camera))
			LOG_ERROR("failed to update camera matrixes");
		render_copy_cameras(g_wow->cull_frame, g_wow->frustum_camera, g_wow->view_camera);
		JKS_HMAP_FOREACH(iter, g_wow->objects)
		{
			struct object *obj = *(struct object**)jks_hmap_iterator_get_value(&iter);
			if (object_is_worldobj(obj))
				worldobj_add_to_render((struct worldobj*)obj);
		}
		if (g_wow->map && culled_async)
			loader_start_cull(g_wow->loader);
		if (g_wow->map && (!g_wow->interface || !g_wow->interface->is_gluescreen))
			map_render(g_wow->map, g_wow->draw_frame);
		gfx_bind_render_target(g_wow->device, NULL);
		gfx_set_viewport(g_wow->device, 0, 0, g_wow->render_width, g_wow->render_height);
		gfx_set_scissor(g_wow->device, 0, 0, g_wow->render_width, g_wow->render_height);
		gfx_bind_pipeline_state(g_wow->device, &pipeline_state);
		gfx_clear_depth_stencil(g_wow->device, NULL, 1, 0);
		if (!g_wow->map || (g_wow->interface && g_wow->interface->is_gluescreen))
			gfx_clear_color(g_wow->device, NULL, GFX_RENDERTARGET_ATTACHMENT_COLOR0, (struct vec4f){0, 0, 0, 1});
		if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
			interface_render(g_wow->interface);
		if (g_wow->wow_opt & WOW_OPT_RENDER_GUI)
			lagometer_draw(g_wow->lagometer);
		ended = nanotime();
		g_wow->last_frame_draw_duration = ended - started;
		started = nanotime();
		net_tick(g_wow->network);
		gfx_window_poll_events(g_wow->window);
		ended = nanotime();
		g_wow->last_frame_events_duration = ended - started;
		if (g_wow->map && !culled_async)
		{
			started = nanotime();
			map_cull(g_wow->map, g_wow->cull_frame);
			ended = nanotime();
			g_wow->last_frame_cull_duration = ended - started;
		}
		started = nanotime();
		gfx_window_swap_buffers(g_wow->window);
		ended = nanotime();
		g_wow->last_frame_update_duration = ended - started;
		++fps;
		if (g_wow->map && culled_async)
		{
			started = nanotime();
			loader_wait_cull(g_wow->loader);
			ended = nanotime();
			g_wow->last_frame_cull_duration = ended - started;
		}
		started = nanotime();
		loader_tick(g_wow->loader);
		gfx_device_tick(g_wow->device);
		render_gc(g_wow->draw_frame);
		g_wow->current_frame++;
		ended = nanotime();
		g_wow->last_frame_misc_duration = ended - started;
	}
	gfx_delete_depth_stencil_state(g_wow->device, &depth_stencil_state);
	gfx_delete_rasterizer_state(g_wow->device, &rasterizer_state);
	gfx_delete_blend_state(g_wow->device, &blend_state);
	gfx_delete_input_layout(g_wow->device, &input_layout);
	gfx_delete_pipeline_state(g_wow->device, &pipeline_state);
}

void update_world_map_data(int32_t continent_id, int32_t zone_id);

static int wow_main(int ac, char **av)
{
	if (!cvars_init())
	{
		LOG_ERROR("failed to init cvars");
		return EXIT_FAILURE;
	}
	g_wow->social = social_new();
	if (!g_wow->social)
	{
		LOG_ERROR("failed to create social");
		return EXIT_FAILURE;
	}
	g_wow->group = group_new();
	if (!g_wow->group)
	{
		LOG_ERROR("failed to create group");
		return EXIT_FAILURE;
	}
	g_wow->objects = mem_malloc(MEM_GENERIC, sizeof(*g_wow->objects));
	if (!g_wow->objects)
	{
		LOG_ERROR("malloc failed");
		return EXIT_FAILURE;
	}
	jks_hmap_init(g_wow->objects, sizeof(struct object*), NULL, jks_hmap_hash_u64, jks_hmap_cmp_u64, &jks_hmap_memory_fn_GENERIC);
	g_wow->db = db_new();
	if (!g_wow->db)
	{
		LOG_ERROR("malloc failed");
		return EXIT_FAILURE;
	}
	g_wow->starttime = nanotime();
	g_wow->wow_opt |= WOW_OPT_ASYNC_CULL;
	g_wow->draw_frame_id = wow_frame_id(0);
	g_wow->cull_frame_id = wow_frame_id(1);
	g_wow->gc_frame_id = wow_frame_id(-1);
	g_wow->draw_frame = wow_frame(0);
	g_wow->cull_frame = wow_frame(1);
	g_wow->gc_frame = wow_frame(-1);
	uint32_t mapid = 530;
	const char *screen = NULL;
	char opt;
	const char *renderer = NULL;
	const char *windowing = NULL;
	enum gfx_window_backend window_backend;

	if (gfx_has_device_backend(GFX_DEVICE_GL4))
		renderer = "gl4";
	else if (gfx_has_device_backend(GFX_DEVICE_GL3))
		renderer = "gl3";
	else if (gfx_has_device_backend(GFX_DEVICE_VK))
		renderer = "vk";
	else if (gfx_has_device_backend(GFX_DEVICE_D3D11))
		renderer = "d3d11";
	else if (gfx_has_device_backend(GFX_DEVICE_D3D9))
		renderer = "d3d9";
	else if (gfx_has_device_backend(GFX_DEVICE_GLES3))
		renderer = "gles3";

	if (!renderer)
	{
		LOG_ERROR("no device backend available");
		return EXIT_FAILURE;
	}

	if (gfx_has_window_backend(GFX_WINDOW_X11))
		windowing = "x11";
	else if (gfx_has_window_backend(GFX_WINDOW_WIN32))
		windowing = "win32";
	else if (gfx_has_window_backend(GFX_WINDOW_WAYLAND))
		windowing = "wayland";
	else if (gfx_has_window_backend(GFX_WINDOW_GLFW))
		windowing = "glfw";

	if (!windowing)
	{
		LOG_ERROR("no window backend available");
		return EXIT_FAILURE;
	}

	while ((opt = getopt(ac, av, "hm:p:ex:w:l:s:")) != -1)
	{
		switch (opt)
		{
			case 'h':
				printf("wow [-h] [-m <mapid>] [-p <path>] [-e] [-x <device>] [-w <window>] [-l <locale>] [-s <screen>]\n");
				printf("-h: show this help\n");
				printf("-m: set set mapid where to spawn\n");
				printf("-p: set the game path\n");
				printf("-x: set the render engine:\n");
				if (gfx_has_device_backend(GFX_DEVICE_GL3))
					printf("\tgl3: OpenGL 3\n");
				if (gfx_has_device_backend(GFX_DEVICE_GL4))
					printf("\tgl4: OpenGL 4\n");
				if (gfx_has_device_backend(GFX_DEVICE_D3D9))
					printf("\td3d9: Direct3D 9\n");
				if (gfx_has_device_backend(GFX_DEVICE_D3D11))
					printf("\td3d11: Direct3D 11\n");
				if (gfx_has_device_backend(GFX_DEVICE_VK))
					printf("\tvk: Vulkan\n");
				if (gfx_has_device_backend(GFX_DEVICE_GLES3))
					printf("\tgles3: OpenGL ES 3\n");
				printf("-w: set the windowing framework:\n");
				if (gfx_has_window_backend(GFX_WINDOW_X11))
					printf("\tx11: Native x.org\n");
				if (gfx_has_window_backend(GFX_WINDOW_WIN32))
					printf("\twin32: Native windows\n");
				if (gfx_has_window_backend(GFX_WINDOW_WAYLAND))
					printf("\twayland: Native wayland\n");
				if (gfx_has_window_backend(GFX_WINDOW_GLFW))
					printf("\tglfw: glfw library\n");
				printf("-l: set the locale (frFR, enUS, ..)\n");
				printf("-s: set the boot screen (FrameXML, GlueXML)\n");
				return EXIT_SUCCESS;
			case 'm':
				mapid = atoll(optarg);
				break;
			case 'p':
				g_wow->game_path = optarg;
				break;
			case 'x':
				renderer = optarg;
				break;
			case 'w':
				windowing = optarg;
				break;
			case 'l':
				g_wow->locale = optarg;
				break;
			case 's':
				screen = optarg;
				break;
			default:
				LOG_ERROR("unknown parameter: %c", opt);
				return EXIT_FAILURE;
		}
	}
	if (!setup_game_files())
	{
		LOG_ERROR("failed to setup game files");
		return EXIT_FAILURE;
	}
	if (!load_trs())
	{
		LOG_ERROR("failed to load TRS");
		return EXIT_FAILURE;
	}
	if (!setup_cache())
	{
		LOG_ERROR("failed to setup cache");
		return EXIT_FAILURE;
	}
	if (gfx_has_window_backend(GFX_WINDOW_X11) && !strcmp(windowing, "x11"))
		window_backend = GFX_WINDOW_X11;
	else if (gfx_has_window_backend(GFX_WINDOW_WIN32) && !strcmp(windowing, "win32"))
		window_backend = GFX_WINDOW_WIN32;
	else if (gfx_has_window_backend(GFX_WINDOW_WAYLAND) && !strcmp(windowing, "wayland"))
		window_backend = GFX_WINDOW_WAYLAND;
	else if (gfx_has_window_backend(GFX_WINDOW_GLFW) && !strcmp(windowing, "glfw"))
		window_backend = GFX_WINDOW_GLFW;
	else
	{
		LOG_ERROR("unknown window backend: %s", windowing);
		return EXIT_FAILURE;
	}

	if (gfx_has_device_backend(GFX_DEVICE_GL3) && !strcmp(renderer, "gl3"))
	{
		if (!setup_window(window_backend, GFX_DEVICE_GL3))
			return EXIT_FAILURE;
	}
	else if (gfx_has_device_backend(GFX_DEVICE_GL4) && !strcmp(renderer, "gl4"))
	{
		if (!setup_window(window_backend, GFX_DEVICE_GL4))
			return EXIT_FAILURE;
	}
	else if (gfx_has_device_backend(GFX_DEVICE_D3D9) && !strcmp(renderer, "d3d9"))
	{
		if (!setup_window(window_backend, GFX_DEVICE_D3D9))
			return EXIT_FAILURE;
	}
	else if (gfx_has_device_backend(GFX_DEVICE_D3D11) && !strcmp(renderer, "d3d11"))
	{
		if (!setup_window(window_backend, GFX_DEVICE_D3D11))
			return EXIT_FAILURE;
	}
	else if (gfx_has_device_backend(GFX_DEVICE_VK) && !strcmp(renderer, "vk"))
	{
		if (!setup_window(window_backend, GFX_DEVICE_VK))
			return EXIT_FAILURE;
	}
	else if (gfx_has_device_backend(GFX_DEVICE_GLES3) && !strcmp(renderer, "gles3"))
	{
		if (!setup_window(window_backend, GFX_DEVICE_GLES3))
			return EXIT_FAILURE;
	}
	else
	{
		LOG_ERROR("unknown engine: %s", renderer);
		return EXIT_FAILURE;
	}
	if (!setup_shaders())
	{
		LOG_ERROR("failed to setup shaders");
		return EXIT_FAILURE;
	}
	if (!setup_graphics())
	{
		LOG_ERROR("failed to setup graphics");
		return EXIT_FAILURE;
	}
	if (!setup_render_passes())
	{
		LOG_ERROR("failed to setup render passes");
		return EXIT_FAILURE;
	}
	g_wow->gx_frames = mem_malloc(MEM_GENERIC, sizeof(*g_wow->gx_frames) * RENDER_FRAMES_COUNT);
	if (!g_wow->gx_frames)
	{
		LOG_ERROR("failed to allocate render frames");
		return EXIT_FAILURE;
	}
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		init_gx_frame(&g_wow->gx_frames[i]);
	LOG_INFO("loading async loaders");
	g_wow->loader = loader_new();
	if (!g_wow->loader)
	{
		LOG_ERROR("failed to load async loaders");
		return EXIT_FAILURE;
	}
	if (!load_dbc())
	{
		LOG_ERROR("failed to load dbc");
		return EXIT_FAILURE;
	}
#if 0
	0: Azeroth
	1: Kalimdor
	13: Test
	30: Alterac
	33: Ombrecroc
	36: deadmines
	37: PVPZone02
	43: WailingCaverns
	44: Monastery
	47: RazorfenKraul
	48: Blackfathom
	70: Uldaman
	90: Gnomeragon
	109: SunkenTemple
	129: RazorfenDowns
	169: EmeraldDream
	189: MonasteryInstances
	209: TanarisInstance
	229: BlackRockSpire
	230: BlackrockDepths
	249: Onyxia
	269: CavernsOfTime
	289; SchoolofNecromancy
	309: Zul'Gurub
	329: Stratholme
	349: Mauradon
	369: DeeprunTram
	389: OrgrimmarInstance
	409: MoltenCore
	429: DireMaul
	449: AlliancePVPBarracks
	450: HordePVPBarracks
	451: dev
	469: BlackwingLair
	489: Warsong
	509: AhnQiraj
	529: Arathi
	530: Outland
	531: AhnQirajTemple
	532: Karazahn
	533: Stratholme Raid
	534: HyjalPast
	540: HellfireMilitary
	542: HellfireDemon
	543: HellfireRampart
	544: HellfireRaid
	545: CoilfangPumping
	546: CoilfangMarsh
	547: CoilfangDraenei
	548: CoilfangRaid
	550: TempestKeepRaid
	552: TempestKeepArcane
	553: TempestKeepAtrium
	554: TempestKeepFactory
	555: AuchindounShadow
	556: AuchindounDemon
	557: AuchindounEthereal
	558: AuchindounDraenei
	559: Nagrand PVP
	560: HillsbradPast
	562: bladesedgearena
	564: BlackTemple
	565: GruulsLair
	566: Netherstorm BG
	568: ZulAman
	572: PVPLordaeron
	580: SunwellPlateau
	585: Sunwell5ManFix
	598: Sunwell5Man
#endif
	int pa_err = Pa_Initialize();
	if (pa_err)
	{
		LOG_ERROR("failed to init snd");
		return EXIT_FAILURE;
	}
	g_wow->snd = snd_new();
	if (!g_wow->snd)
	{
		LOG_ERROR("can't create sound system");
		return EXIT_FAILURE;
	}
	if (!setup_map())
	{
		LOG_ERROR("failed to setup map");
		return EXIT_FAILURE;
	}
	if (!wow_set_map(mapid))
	{
		LOG_ERROR("failed to set map");
		return EXIT_FAILURE;
	}
	if (!setup_network())
	{
		LOG_ERROR("failed to setup network");
		return EXIT_FAILURE;
	}
	if (!setup_interface())
	{
		LOG_ERROR("failed to setup interface");
		return EXIT_FAILURE;
	}
	if (screen)
	{
		if (!strcmp(screen, "FrameXML"))
		{
			g_wow->interface->switch_framescreen = true;
		}
		else if (!strcmp(screen, "GlueXML"))
		{
			g_wow->interface->switch_gluescreen = true;
		}
		else
		{
			LOG_ERROR("unknown screen");
			return EXIT_FAILURE;
		}
	}
	setup_gui();
	g_wow->player = player_new(50);
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, UNIT_FIELD_BYTES_0, (CLASS_WARRIOR << 8) | (RACE_HUMAN << 0) | (1 << 16));
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, UNIT_FIELD_DISPLAYID, 50);
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + 16 * EQUIPMENT_SLOT_HEAD,      30972);
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + 16 * EQUIPMENT_SLOT_SHOULDERS, 30979);
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + 16 * EQUIPMENT_SLOT_CHEST,     30975);
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + 16 * EQUIPMENT_SLOT_LEGS,      30978);
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + 16 * EQUIPMENT_SLOT_FEET,      32345);
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + 16 * EQUIPMENT_SLOT_HANDS,     30969);
#if 1
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, UNIT_FIELD_MOUNTDISPLAYID, 17890);
#endif
#if 0
	object_fields_set_u32(&((struct object*)g_wow->player)->fields, UNIT_FIELD_MOUNTDISPLAYID, 20932);
#endif
	g_wow->cameras[0] = camera_new();
	if (!g_wow->cameras[0])
	{
		LOG_ERROR("failed to setup main camera");
		return EXIT_FAILURE;
	}
	g_wow->cameras[0]->worldobj = (struct worldobj*)g_wow->player;
#if 1
	((struct worldobj*)g_wow->player)->movement_data.flags |= MOVEFLAG_CAN_FLY;
#endif
	g_wow->cameras[1] = camera_new();
	if (!g_wow->cameras[1])
	{
		LOG_ERROR("failed to setup second camera");
		return EXIT_FAILURE;
	}
	g_wow->view_camera = g_wow->cameras[0];
	g_wow->frustum_camera = g_wow->cameras[0];
	{ /* XXX: try not to do this ? */
		struct gfx_resize_event event;
		event.width = g_wow->render_width;
		event.height = g_wow->render_height;
		resize_callback(&event);
	}
	update_world_map_data(2, 4); /* XXX remove this */
	if (cache_ref_by_key_font(g_wow->cache, "FONTS\\FRIZQT__.TTF", &g_wow->font_model_3d))
	{
		g_wow->font_3d = font_new(g_wow->font_model_3d, 50, 1, NULL);
		if (g_wow->font_3d)
			g_wow->font_3d->atlas->bpp = 8;
		else
			LOG_WARN("failed to create 3d font");
	}
	else
	{
		LOG_WARN("font not found");
		g_wow->font_3d = NULL;
	}
	//snd_set_glue_music(g_wow->snd, "SOUND\\MUSIC\\GLUESCREENMUSIC\\BC_MAIN_THEME.MP3");
	loop();
	map_setid(g_wow->map, -1);
	unload_dbc();
	map_delete(g_wow->map);
	object_delete((struct object*)g_wow->player);
	interface_delete(g_wow->interface);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		render_gc(&g_wow->gx_frames[i]);
	network_delete(g_wow->network);
	lagometer_delete(g_wow->lagometer);
	while (1)
	{
		bool done = false;
		for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
			render_gc(&g_wow->gx_frames[i]);
		while (loader_has_loading(g_wow->loader) || loader_has_gc(g_wow->loader))
		{
			done = true;
			loader_tick(g_wow->loader);
			g_wow->draw_frame_id = (g_wow->draw_frame_id + 1) % RENDER_FRAMES_COUNT;
		}
		while (loader_has_async(g_wow->loader))
		{
			done = true;
			usleep(50000);
		}
		if (!done)
			break;
	}
	render_target_delete(g_wow->post_process.dummy1);
	render_target_delete(g_wow->post_process.dummy2);
	render_target_delete(g_wow->post_process.msaa);
	render_pass_delete(g_wow->post_process.sharpen);
	render_pass_delete(g_wow->post_process.bloom);
	render_pass_delete(g_wow->post_process.sobel);
	render_pass_delete(g_wow->post_process.ssao);
	render_pass_delete(g_wow->post_process.glow);
	render_pass_delete(g_wow->post_process.fxaa);
	render_pass_delete(g_wow->post_process.fsaa);
	render_pass_delete(g_wow->post_process.cel);
	font_delete(g_wow->font_3d);
	cache_unref_by_ref_font(g_wow->cache, g_wow->font_model_3d);
	cache_print(g_wow->cache);
	cache_delete(g_wow->cache);
	shaders_clean(g_wow->shaders);
	mem_free(MEM_GENERIC, g_wow->shaders);
	graphics_clear(g_wow->graphics);
	mem_free(MEM_GENERIC, g_wow->graphics);
	/* XXX: wait for async end */
	gfx_device_tick(g_wow->device); /* finish gc cycle */
	camera_delete(g_wow->cameras[0]);
	camera_delete(g_wow->cameras[1]);
	loader_delete(g_wow->loader);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		destroy_gx_frame(&g_wow->gx_frames[i]);
	mem_free(MEM_GENERIC, g_wow->gx_frames);
	gfx_delete_window(g_wow->window);
	wow_mpq_compound_delete(g_wow->mpq_compound);
	jks_array_destroy(g_wow->mpq_archives);
	jks_hmap_destroy(g_wow->objects);
	db_delete(g_wow->db);
	mem_free(MEM_GENERIC, g_wow->mpq_archives);
	mem_free(MEM_GENERIC, g_wow->objects);
	social_delete(g_wow->social);
	group_delete(g_wow->group);
	snd_delete(g_wow->snd);
	cvars_cleanup();
	return EXIT_SUCCESS;
}

static void resize_callback(struct gfx_resize_event *event)
{
	g_wow->render_width = event->width;
	g_wow->render_height = event->height;
	dirty_render_passes();
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
		interface_on_window_resized(g_wow->interface, event);
}

static void key_down_callback(struct gfx_key_event *event)
{
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
	{
		if (interface_on_key_down(g_wow->interface, event))
			return;
	}
#define RENDER_OPT_FLIP(id) \
do \
{ \
	if (g_wow->render_opt & id) \
		g_wow->render_opt &= ~id; \
	else \
		g_wow->render_opt |= id; \
} while (0)

#define WOW_OPT_FLIP(id) \
do \
{ \
	if (g_wow->wow_opt & id) \
		g_wow->wow_opt &= ~id; \
	else \
		g_wow->wow_opt |= id; \
} while (0)

	switch (event->key)
	{
		case GFX_KEY_9:
			exit(EXIT_SUCCESS);
			break;
		case GFX_KEY_ESCAPE:
			g_wow->wow_opt &= ~WOW_OPT_FOCUS_3D;
			gfx_window_ungrab_cursor(g_wow->window);
			return;
		case GFX_KEY_LBRACKET:
			RENDER_OPT_FLIP(RENDER_OPT_M2_COLLISIONS);
			RENDER_OPT_FLIP(RENDER_OPT_WMO_COLLISIONS);
			return;
		case GFX_KEY_RBRACKET:
			RENDER_OPT_FLIP(RENDER_OPT_COLLISIONS);
			return;
		case GFX_KEY_P:
			RENDER_OPT_FLIP(RENDER_OPT_WMO_PORTALS);
			return;
		case GFX_KEY_O:
			RENDER_OPT_FLIP(RENDER_OPT_WMO_AABB);
			return;
		case GFX_KEY_I:
			RENDER_OPT_FLIP(RENDER_OPT_M2_AABB);
			return;
		case GFX_KEY_L:
			RENDER_OPT_FLIP(RENDER_OPT_WMO);
			return;
		case GFX_KEY_K:
			RENDER_OPT_FLIP(RENDER_OPT_M2);
			return;
		case GFX_KEY_Y:
			RENDER_OPT_FLIP(RENDER_OPT_WMO_LIGHTS);
			RENDER_OPT_FLIP(RENDER_OPT_M2_LIGHTS);
			return;
		case GFX_KEY_T:
			RENDER_OPT_FLIP(RENDER_OPT_TAXI);
			return;
		case GFX_KEY_R:
			RENDER_OPT_FLIP(RENDER_OPT_ADT_AABB);
			RENDER_OPT_FLIP(RENDER_OPT_MCNK_AABB);
			RENDER_OPT_FLIP(RENDER_OPT_MCLQ_AABB);
			return;
		case GFX_KEY_F:
			RENDER_OPT_FLIP(RENDER_OPT_FOG);
			return;
		case GFX_KEY_U:
			RENDER_OPT_FLIP(RENDER_OPT_M2_BONES);
			return;
		case GFX_KEY_Z:
			RENDER_OPT_FLIP(RENDER_OPT_WDL);
			return;
		case GFX_KEY_X:
			RENDER_OPT_FLIP(RENDER_OPT_MCLQ);
			return;
		case GFX_KEY_V:
			g_wow->post_process.glow->enabled = !g_wow->post_process.glow->enabled;
			return;
		case GFX_KEY_B:
			WOW_OPT_FLIP(WOW_OPT_VSYNC);
			gfx_window_set_swap_interval(g_wow->window, (g_wow->wow_opt & WOW_OPT_VSYNC) ? -1 : 0);
			return;
		case GFX_KEY_N:
			render_target_set_enabled(g_wow->post_process.msaa, !g_wow->post_process.msaa->enabled);
			return;
		case GFX_KEY_M:
			RENDER_OPT_FLIP(RENDER_OPT_MESH);
			graphics_build_world_rasterizer_states(g_wow->graphics);
			return;
		case GFX_KEY_COMMA:
			g_wow->post_process.sobel->enabled = !g_wow->post_process.sobel->enabled;
			g_wow->post_process.cel->enabled = !g_wow->post_process.cel->enabled;
			return;
		case GFX_KEY_PERIOD:
			g_wow->post_process.fxaa->enabled = !g_wow->post_process.fxaa->enabled;
			return;
		case GFX_KEY_SLASH:
			g_wow->post_process.ssao->enabled = !g_wow->post_process.ssao->enabled;
			return;
		case GFX_KEY_APOSTROPHE:
			g_wow->post_process.bloom->enabled = !g_wow->post_process.bloom->enabled;
			break;
		case GFX_KEY_BACKSLASH:
			g_wow->post_process.sharpen->enabled = !g_wow->post_process.sharpen->enabled;
			break;
		case GFX_KEY_SEMICOLON:
			g_wow->post_process.chromaber->enabled = !g_wow->post_process.chromaber->enabled;
			break;
		/* KP */
		case GFX_KEY_PAGE_UP:
			g_wow->view_camera->view_distance += CHUNK_WIDTH * (event->mods & GFX_KEY_MOD_CONTROL ? 10 : 1) * (event->mods & GFX_KEY_MOD_SHIFT ? 5 : 1);
			return;
		case GFX_KEY_PAGE_DOWN:
			g_wow->view_camera->view_distance -= CHUNK_WIDTH * (event->mods & GFX_KEY_MOD_CONTROL ? 10 : 1) * (event->mods & GFX_KEY_MOD_SHIFT ? 5 : 1);
			return;
		case GFX_KEY_H:
			WOW_OPT_FLIP(WOW_OPT_RENDER_GUI);
			return;
		case GFX_KEY_G:
			WOW_OPT_FLIP(WOW_OPT_RENDER_INTERFACE);
			return;
		case GFX_KEY_EQUAL:
			WOW_OPT_FLIP(WOW_OPT_DIFFERENT_CAMERAS);
			g_wow->view_camera = g_wow->cameras[!!(g_wow->wow_opt & WOW_OPT_DIFFERENT_CAMERAS)];
			return;
		case GFX_KEY_F5:
			wow_set_map(0);
			return;
		case GFX_KEY_F6:
			wow_set_map(1);
			return;
		case GFX_KEY_F7:
			wow_set_map(530);
			return;
		case GFX_KEY_F9:
		{
			const char *username = "ADMINISTRATOR";
			const char *password = "ADMINISTRATOR";
			net_auth_connect(g_wow->network, username, password);
			return;
		}
		case GFX_KEY_F10:
			WOW_OPT_FLIP(WOW_OPT_GRAVITY);
			break;
		case GFX_KEY_F12:
			shaders_clean(g_wow->shaders);
			shaders_build(g_wow->shaders);
			break;
		case GFX_KEY_1:
			g_wow->view_camera->fov += 1 / 180. * M_PI;
			return;
		case GFX_KEY_2:
			g_wow->view_camera->fov -= 1 / 180. * M_PI;
			return;
		case GFX_KEY_3:
			WOW_OPT_FLIP(WOW_OPT_AABB_OPTIMIZE);
			return;
		case GFX_KEY_4:
			WOW_OPT_FLIP(WOW_OPT_ASYNC_CULL);
			return;
		case GFX_KEY_5:
			g_wow->fsaa -= 0.1;
			if (g_wow->fsaa <= 0.1)
				g_wow->fsaa = 0.1;
			dirty_render_passes();
			break;
		case GFX_KEY_6:
			g_wow->fsaa += 0.1;
			if (g_wow->fsaa > 4)
				g_wow->fsaa = 4;
			dirty_render_passes();
			break;
		default:
			break;
	}
#undef WORLD_OPTION_FLIP
}

static void key_up_callback(struct gfx_key_event *event)
{
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
	{
		if (interface_on_key_up(g_wow->interface, event))
			return;
	}
}

static void key_press_callback(struct gfx_key_event *event)
{
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
	{
		if (interface_on_key_press(g_wow->interface, event))
			return;
	}
}

static void char_callback(struct gfx_char_event *event)
{
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
	{
		if (interface_on_char(g_wow->interface, event))
			return;
	}
}

static void mouse_down_callback(struct gfx_mouse_event *event)
{
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
	{
		if (interface_on_mouse_down(g_wow->interface, event))
			return;
	}
	if (!(g_wow->wow_opt & WOW_OPT_FOCUS_3D))
	{
		g_wow->view_camera->move_unit = false;
		switch (event->button)
		{
			case GFX_MOUSE_BUTTON_RIGHT:
				g_wow->view_camera->move_unit = true;
				/* FALLTHROUGH */
			case GFX_MOUSE_BUTTON_LEFT:
				g_wow->wow_opt |= WOW_OPT_FOCUS_3D;
				gfx_window_grab_cursor(g_wow->window);
				break;
			default:
				break;
		}
	}
}

static void mouse_up_callback(struct gfx_mouse_event *event)
{
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
	{
		if (interface_on_mouse_up(g_wow->interface, event))
			return;
	}
	if ((event->button == GFX_MOUSE_BUTTON_LEFT || event->button == GFX_MOUSE_BUTTON_RIGHT) && (g_wow->wow_opt & WOW_OPT_FOCUS_3D))
	{
		g_wow->view_camera->move_unit = false;
		g_wow->wow_opt &= ~WOW_OPT_FOCUS_3D;
		gfx_window_ungrab_cursor(g_wow->window);
	}
}

static void mouse_move_callback(struct gfx_pointer_event *event)
{
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
		interface_on_mouse_move(g_wow->interface, event);
}

static void mouse_scroll_callback(struct gfx_scroll_event *event)
{
	if ((g_wow->wow_opt & WOW_OPT_RENDER_INTERFACE) && g_wow->interface)
	{
		if (interface_on_mouse_scroll(g_wow->interface, event))
			return;
	}
	camera_handle_scroll(g_wow->view_camera, event->y);
}

static void error_callback(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	LOG_ERRORV(fmt, ap);
	va_end(ap);
}

void wow_set_player(struct player *player)
{
	g_wow->player = player;
	g_wow->cameras[0]->worldobj = (struct worldobj*)player;
}

bool wow_set_object(uint64_t guid, struct object *object)
{
	if (!object)
	{
		jks_hmap_erase(g_wow->objects, JKS_HMAP_KEY_U64(guid));
		return true;
	}
	if (!jks_hmap_set(g_wow->objects, JKS_HMAP_KEY_U64(guid), &object))
	{
		LOG_ERROR("failed to add object to hmap");
		return false;
	}
	return true;
}

struct object *wow_get_object(uint64_t guid)
{
	struct object **object = jks_hmap_get(g_wow->objects, JKS_HMAP_KEY_U64(guid));
	if (!object)
		return NULL;
	return *object;
}

int64_t nanotime(void)
{
#ifdef __linux__
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	uint64_t t = ts.tv_sec * 1000000000 + ts.tv_nsec;
	return t - g_wow->starttime;
#elif defined (__unix__)
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint64_t t = ts.tv_sec * 1000000000 + ts.tv_nsec;
	return t - g_wow->starttime;
#elif defined (_WIN32)
	LARGE_INTEGER ret;
	QueryPerformanceCounter(&ret);
	return ret.QuadPart / performance_frequency;
#else
#error unsupported nanotime
#endif
}

void normalize_mpq_filename(char *filename, size_t size)
{
	(void)size;
	size_t j = 0;
	for (size_t i = 0; filename[i]; ++i, ++j)
	{
		if (filename[i] == '/')
			filename[j] = '\\';
		else
			filename[j] = toupper(filename[i]);
		if (filename[j] == '\\' && j > 0 && filename[j - 1] == '\\')
		{
			filename[j] = '\0';
			j--;
		}
	}
	filename[j] = '\0';
}

void normalize_m2_filename(char *filename, size_t size)
{
	normalize_mpq_filename(filename, size);
	size_t len = strlen(filename);
	if (len >= 3
	 && (filename[len - 1] == 'X' || filename[len - 1] == 'L')
	 &&  filename[len - 2] == 'D' && filename[len - 3] == 'M')
	{
		filename[len - 2] = '2';
		filename[len - 1] = '\0';
	}
}

void normalize_blp_filename(char *filename, size_t size)
{
	if (!filename[0])
		return;
	normalize_mpq_filename(filename, size);
	size_t len = strlen(filename);
	if (len < 4
	 || filename[len - 1] != 'P'
	 || filename[len - 2] != 'L'
	 || filename[len - 3] != 'B'
	 || filename[len - 4] != '.')
		snprintf(filename + len, size - len, ".BLP");
}

void normalize_wmo_filename(char *filename, size_t size)
{
	normalize_mpq_filename(filename, size);
}

bool wow_load_compound(struct wow_mpq_compound *compound)
{
	for (size_t i = 0; i < g_wow->mpq_archives->size; ++i)
	{
		struct wow_mpq_archive *archive = *(struct wow_mpq_archive**)jks_array_get(g_wow->mpq_archives, i);
		if (!wow_mpq_compound_add_archive(compound, archive))
			return false;
	}
	return true;
}

struct gx_frame *wow_frame(int offset)
{
	return &g_wow->gx_frames[wow_frame_id(offset)];
}

int wow_frame_id(int offset)
{
	int id = g_wow->current_frame + offset;
	if (id < 0)
		id += RENDER_FRAMES_COUNT;
	return id % RENDER_FRAMES_COUNT;
}

int wow_asprintf(int memory_type, char **str, const char *fmt, ...)
{
	int ret;
	va_list args;
	va_start(args, fmt);
	ret = wow_vasprintf(memory_type, str, fmt, args);
	va_end(args);
	return ret;
}

int wow_vasprintf(int memory_type, char **str, const char *fmt, va_list args)
{
	int size;
	va_list tmp;
	va_copy(tmp, args);
	size = vsnprintf(NULL, 0, fmt, tmp);
	va_end(tmp);
	if (size < 0)
		return size;
	*str = mem_malloc(memory_type, size + 1);
	if (*str == NULL)
		return -1;
	return vsnprintf(*str, size + 1, fmt, args);
}

static void *freetype_malloc(FT_Memory memory, long size)
{
	(void)memory;
	return mem_malloc(MEM_FONT, size);
}

static void *freetype_realloc(FT_Memory memory, long cur_size, long new_size, void *block)
{
	(void)memory;
	(void)cur_size;
	return mem_realloc(MEM_FONT, block, new_size);
}

static void freetype_free(FT_Memory memory, void *block)
{
	(void)memory;
	mem_free(MEM_FONT, block);
}

int main(int ac, char **av)
{
	mem_init();
#ifdef __unix__
	g_log_colored = isatty(1);
#else
	g_log_colored = false;
#endif
	setvbuf(stdout, NULL , _IOLBF , 4096 * 2);
	setvbuf(stderr, NULL , _IOLBF , 4096 * 2);
	srand(time(NULL));
	gfx_memory.malloc = mem_malloc_GFX;
	gfx_memory.realloc = mem_realloc_GFX;
	gfx_memory.free = mem_free_GFX;
	wow_memory.malloc = mem_malloc_LIBWOW;
	wow_memory.realloc = mem_realloc_LIBWOW;
	wow_memory.free = mem_free_LIBWOW;
	gfx_error_callback = error_callback;
	g_wow = mem_zalloc(MEM_GENERIC, sizeof(*g_wow));
	if (!g_wow)
	{
		LOG_ERROR("failed to alloc g_wow");
		return EXIT_FAILURE;
	}
	g_wow->game_path = "WoW";
	g_wow->locale = "frFR";
	g_wow->anisotropy = 16;
	g_wow->fsaa = 1;
	g_wow->render_opt |= RENDER_OPT_MCNK;
	g_wow->render_opt |= RENDER_OPT_MCLQ;
	g_wow->render_opt |= RENDER_OPT_WMO;
	g_wow->render_opt |= RENDER_OPT_WDL;
	g_wow->render_opt |= RENDER_OPT_M2;
	g_wow->render_opt |= RENDER_OPT_FOG;
	g_wow->render_opt |= RENDER_OPT_WMO_LIQUIDS;
	g_wow->render_opt |= RENDER_OPT_SKYBOX;
	g_wow->render_opt |= RENDER_OPT_M2_PARTICLES;
	g_wow->render_opt |= RENDER_OPT_M2_RIBBONS;
#if 0
	g_wow->render_opt |= RENDER_OPT_SSR;
#endif
	g_wow->wow_opt |= WOW_OPT_RENDER_INTERFACE;
	g_wow->wow_opt |= WOW_OPT_AABB_OPTIMIZE;
	g_wow->wow_opt |= WOW_OPT_RENDER_GUI;
#if defined (_WIN32)
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		performance_frequency = frequency.QuadPart / 1000000000.;
	}
#endif
	g_wow->ft_memory = mem_malloc(MEM_FONT, sizeof(*g_wow->ft_memory));
	if (!g_wow->ft_memory)
	{
		LOG_ERROR("failed to allocate freetype memory");
		return EXIT_FAILURE;
	}
	g_wow->ft_memory->alloc = freetype_malloc;
	g_wow->ft_memory->realloc = freetype_realloc;
	g_wow->ft_memory->free = freetype_free;
	if (FT_New_Library(g_wow->ft_memory, &g_wow->ft_lib))
	{
		LOG_ERROR("failed to init freetype lib");
		return EXIT_FAILURE;
	}
	FT_Add_Default_Modules(g_wow->ft_lib);
	wow_mpq_init_crypt_table();
	int ret = wow_main(ac, av);
	FT_Done_Library(g_wow->ft_lib);
	mem_free(MEM_FONT, g_wow->ft_memory);
	mem_free(MEM_GENERIC, g_wow);
	mem_dump();
	return ret;
}
