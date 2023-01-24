include config

ifeq ($(TARGET), windows)

NAME = wow.exe

else

NAME = wow

endif

ifeq ($(TARGET), windows)

HOST = x86_64-w64-mingw32-

else

ifeq ($(TARGET), linux)

HOST = 

else

$(error Must define TARGET as either windows or linux)

endif

endif

CC = $(HOST)gcc
CXX = $(HOST)g++

SL_LIBS = zlib
SL_LIBS+= libpng
SL_LIBS+= freetype
SL_LIBS+= glfw
SL_LIBS+= lua
SL_LIBS+= libxml2
SL_LIBS+= jks
SL_LIBS+= gfx
SL_LIBS+= libwow
SL_LIBS+= portaudio
SL_LIBS+= libsamplerate
SL_LIBS+= jkssl

LDFLAGS+= -fwhole-program

CFLAGS+= -Wall -Wextra
CFLAGS+= -Wshadow
CFLAGS+= -Wunused
CFLAGS+= -pipe
CFLAGS+= -g
CFLAGS+= -march=native
CFLAGS+= -D__STDC_FORMAT_MACROS
CFLAGS+= -Wno-cpp

#XXX: Workaround
CFLAGS+= -Wno-missing-field-initializers

CPPFLAGS+= -DRENDER_FRAMES_COUNT=3

ifeq ($(WITH_PERFORMANCE), YES)

CFLAGS+= -DWITH_PERFORMANCE

endif

ifeq ($(WITH_MEMORY), YES)

CFLAGS+= -DWITH_MEMORY

endif

ifeq ($(TARGET), linux)

LDFLAGS+= -rdynamic

endif

ifeq ($(TARGET), windows)

CFLAGS+= -Wl,-subsystem,windows
LDFLAGS+= -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
#LDFLAGS+= -static

endif

ifeq ($(WITH_LTO), YES)

CFLAGS+= -flto=4
LDFLAGS+= -flto=4

endif

ifeq ($(BUILD), DEBUG)

CFLAGS+= -fsanitize=address
#CFLAGS+= -fsanitize=leak
CFLAGS+= -fsanitize=undefined
#CFLAGS+= -fno-sanitize=vptr
#CFLAGS+= -fsanitize=thread
CFLAGS+= -O0

LDFLAGS+= -fsanitize=address
LDFLAGS+= -fsanitize=undefined
#LDFLAGS+= -fsanitize=thread

else

ifeq ($(BUILD), DEVEL)

CFLAGS += -O0

else

CFLAGS+= -Ofast

endif

endif

ifeq ($(BUILD), RELEASE)

CPPFLAGS+= -DNDEBUG
CPPFLAGS+= -DDBG_NO_FILE

endif

ifeq ($(WITH_LTO), YES)

LTO_BUILD = _LTO

endif

LIB_DIR = lib/build_$(TARGET)_$(BUILD)$(LTO_BUILD)

INCLUDES = -I src
INCLUDES+= -I $(LIB_DIR)/include
INCLUDES+= -I $(LIB_DIR)/include/libxml2
INCLUDES+= -I $(LIB_DIR)/include/freetype2

LIBRARY = -L $(LIB_DIR)/lib

SRCS_PATH = src

SRCS_NAME = wow.c \
            char_input.c \
            camera.c \
            loader.c \
            performance.c \
            shaders.c \
            dbc.c \
            cache.c \
            log.c \
            lagometer.c \
            blp.c \
            db.c \
            memory.c \
            bc.c \
            cvars.c \
            graphics.c \
            gx/m2.c \
            gx/wmo.c \
            gx/wmo_group.c \
            gx/skybox.c \
            gx/wmo_mliq.c \
            gx/mcnk.c \
            gx/mclq.c \
            gx/wdl.c \
            gx/m2_particles.c \
            gx/frame.c \
            gx/text.c \
            ppe/render_pass.c \
            ppe/ssao_render_pass.c \
            ppe/filter_render_pass.c \
            ppe/bloom_render_pass.c \
            ppe/render_target.c \
            obj/fields.c \
            obj/object.c \
            obj/item.c \
            obj/container.c \
            obj/unit.c \
            obj/player.c \
            obj/creature.c \
            obj/gameobj.c \
            obj/worldobj.c \
            obj/dynobj.c \
            obj/corpse.c \
            itf/interface.c \
            itf/enum.c \
            itf/addon.c \
            ui/object.c \
            ui/font_instance.c \
            ui/region.c \
            ui/layered_region.c \
            ui/font_string.c \
            ui/frame.c \
            ui/button.c \
            ui/texture.c \
            ui/check_button.c \
            ui/color_select.c \
            ui/cooldown.c \
            ui/edit_box.c \
            ui/font.c \
            ui/game_tooltip.c \
            ui/message_frame.c \
            ui/minimap.c \
            ui/model.c \
            ui/scroll_frame.c \
            ui/scrolling_message_frame.c \
            ui/simple_html.c \
            ui/slider.c \
            ui/status_bar.c \
            ui/dress_up_model.c \
            ui/player_model.c \
            ui/model.c \
            ui/tabard_model.c \
            ui/gradient.c \
            ui/color.c \
            ui/tex_coords.c \
            ui/anchor.c \
            ui/dimension.c \
            ui/backdrop.c \
            ui/inset.c \
            ui/value.c \
            ui/model_ffx.c \
            ui/movie_frame.c \
            ui/taxi_route_frame.c \
            ui/world_frame.c \
            ui/shadow.c \
            xml/element.c \
            xml/layout_frame.c \
            xml/frame.c \
            xml/texture.c \
            xml/button.c \
            xml/check_button.c \
            xml/slider.c \
            xml/scroll_frame.c \
            xml/script.c \
            xml/include.c \
            xml/status_bar.c \
            xml/ui.c \
            xml/layers.c \
            xml/scripts.c \
            xml/frames.c \
            xml/anchors.c \
            xml/font_string.c \
            xml/color.c \
            xml/tex_coords.c \
            xml/backdrop.c \
            xml/inset.c \
            xml/font.c \
            xml/edit_box.c \
            xml/shadow.c \
            xml/cooldown.c \
            xml/attributes.c \
            xml/dimension.c \
            xml/value.c \
            xml/game_tooltip.c \
            xml/taxi_route_frame.c \
            xml/world_frame.c \
            xml/minimap.c \
            xml/message_frame.c \
            xml/model.c \
            xml/player_model.c \
            xml/dress_up_model.c \
            xml/tabard_model.c \
            xml/scroll_child.c \
            xml/color_select.c \
            xml/scrolling_message_frame.c \
            xml/resize_bounds.c \
            xml/simple_html.c \
            xml/gradient.c \
            xml/model_ffx.c \
            xml/movie_frame.c \
            lua/lua_script.c \
            lua/misc.c \
            lua/std.c \
            lua/objects.c \
            lua/unit.c \
            lua/pet.c \
            lua/guild.c \
            lua/system.c \
            lua/voice.c \
            lua/party.c \
            lua/input.c \
            lua/audio.c \
            lua/loot.c \
            lua/secure.c \
            lua/battle_ground.c \
            lua/chat.c \
            lua/addon.c \
            lua/lfg.c \
            lua/skill.c \
            lua/quest.c \
            lua/world_map.c \
            lua/arena.c \
            lua/glue.c \
            lua/social.c \
            lua/kb.c \
            lua/pvp.c \
            lua/spell.c \
            lua/talent.c \
            lua/auction.c \
            lua/macro.c \
            lua/inbox.c \
            lua/trade.c \
            lua/combat_log.c \
            lua/craft.c \
            lua/bindings.c \
            lua/taxi.c \
            net/socket.c \
            net/auth_socket.c \
            net/world_socket.c \
            net/packet.c \
            net/network.c \
            net/packet_handler.c \
            net/buffer.c \
            net/pkt/social.c \
            net/pkt/auth.c \
            net/pkt/group.c \
            net/pkt/cache.c \
            net/pkt/object.c \
            net/pkt/misc.c \
            net/pkt/movement.c \
            font/atlas.c \
            font/model.c \
            font/font.c \
            game/social.c \
            game/guild.c \
            game/group.c \
            snd/snd.c \
            snd/mp3.c \
            snd/resample.c \
            snd/stream.c \
            snd/wav.c \
            snd/filter.c \
            map/map.c \
            map/tile.c \
            phys/physics.c \

SRCS = $(addprefix $(SRCS_PATH)/, $(SRCS_NAME))

OBJS_PATH = obj

OBJS_NAME = $(SRCS_NAME:.c=.o)

OBJS = $(addprefix $(OBJS_PATH)/, $(OBJS_NAME))

ifeq ($(WITH_DEBUG_RENDERING), YES)

CPPFLAGS+= -DWITH_DEBUG_RENDERING

SRCS_NAME+= gx/aabb.c \
            gx/wmo_portals.c \
            gx/m2_bones.c \
            gx/wmo_lights.c \
            gx/m2_lights.c \
            gx/m2_collisions.c \
            gx/collisions.c \
            gx/wmo_collisions.c \
            gx/taxi.c \

endif

LIBRARY+= -l:libxml2.a
LIBRARY+= -l:liblua.a
LIBRARY+= -l:libfreetype.a
LIBRARY+= -l:libpng16.a
LIBRARY+= -l:libz.a
LIBRARY+= -l:libgfx.a
LIBRARY+= -l:libglfw3.a
LIBRARY+= -l:libjks.a
LIBRARY+= -l:libwow.a
LIBRARY+= -l:libportaudio.a
LIBRARY+= -l:libsamplerate.a
LIBRARY+= -l:libjkssl.a

CPPFLAGS+= -DLIBXML_STATIC #avoid libxml XMLPUBFUN =  __declspec(dllimport)

ifeq ($(TARGET), linux)

LIBRARY+= -ldl
LIBRARY+= -lGL
LIBRARY+= -lX11 -lXcursor -lXi
LIBRARY+= -lvulkan
LIBRARY+= -lasound

endif

ifeq ($(TARGET), windows)

LIBRARY+= -lws2_32
LIBRARY+= -lcrypt32
LIBRARY+= -lwinmm
LIBRARY+= -lole32
LIBRARY+= -lopengl32
LIBRARY+= -lgdi32
LIBRARY+= -ldxgi
LIBRARY+= -ld3d9
LIBRARY+= -ld3d11
LIBRARY+= -ld3dcompiler

endif

LIBRARY+= -lpthread
LIBRARY+= -lm
LIBRARY+= -static-libgcc

all: $(NAME)

$(NAME): $(OBJS)
	@echo "LD $(NAME)"
	@$(CXX) $(LDFLAGS) -o $(NAME) $^ $(LIBRARY)

$(OBJS_PATH)/%.o: $(SRCS_PATH)/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -std=gnu11 $(CPPFLAGS) -o $@ -c $< $(INCLUDES)

size:
	@wc `find -L $(SRCS_PATH) -type f \( -name \*.c -o -name \*.h \)` | tail -n 1

objsize:
	@wc -c `find $(OBJS_PATH) -type f` | tail -n 1

clean:
	@rm -f $(OBJS)
	@rm -f $(NAME)

lib:
	@cd lib/sl_lib && SL_LIBS="$(SL_LIBS)" CFLAGS="$(CFLAGS)" sh build.sh -dxb -t "$(TARGET)_64" -m static -o "$(PWD)/$(LIB_DIR)" -j6

.PHONY: clean size objsize lib
