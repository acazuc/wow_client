#include "net/packets.h"

#include "net/packet.h"

#include "obj/container.h"
#include "obj/creature.h"
#include "obj/gameobj.h"
#include "obj/object.h"
#include "obj/player.h"
#include "obj/dynobj.h"
#include "obj/corpse.h"
#include "obj/item.h"
#include "obj/unit.h"

#include "const.h"
#include "log.h"
#include "wow.h"

#include <inttypes.h>

bool net_smsg_update_object(struct net_packet_reader *packet)
{
	uint32_t objects_count;
	uint8_t on_transport;
	if (!net_read_u32(packet, &objects_count)
	 || !net_read_u8(packet, &on_transport))
		return false;
	for (uint32_t i = 0; i < objects_count; ++i)
	{
		uint8_t update_type;
		if (!net_read_u8(packet, &update_type))
			return false;
		switch ((enum object_update_type)update_type)
		{
			case OBJECT_UPDATE:
			{
				uint64_t guid;
				if (!net_read_guid(packet, &guid))
					return false;
				struct object *object = wow_get_object(guid);
				if (!object)
				{
					LOG_ERROR("object not found: %" PRIu64, guid);
					return false;
				}
				object_read_fields(object, packet);
				break;
			}
			case OBJECT_UPDATE_MOVEMENT:
			{
				LOG_ERROR("unhandled OBJECT_UPDATE_MOVEMENT");
				break;
			}
			case OBJECT_CREATE:
			{
				uint64_t guid;
				if (!net_read_guid(packet, &guid))
					return false;
				if (wow_get_object(guid))
				{
					LOG_ERROR("OBJECT_CREATE already existing object: %" PRIu64, guid);
					return false;
				}
				uint8_t object_type;
				if (!net_read_u8(packet, &object_type))
					return false;
				struct object *object;
				switch ((enum object_type)object_type)
				{
					case OBJECT_OBJECT:
						object = object_new(guid);
						break;
					case OBJECT_ITEM:
						LOG_INFO("new item %" PRIu64, guid);
						object = (struct object*)item_new(guid);
						break;
					case OBJECT_CONTAINER:
						object = (struct object*)container_new(guid);
						break;
					case OBJECT_UNIT:
						object = (struct object*)creature_new(guid);
						break;
					case OBJECT_PLAYER:
						object = (struct object*)player_new(guid);
						break;
					case OBJECT_GAMEOBJECT:
						object = (struct object*)gameobj_new(guid);
						break;
					case OBJECT_DYNAMICOBJECT:
						object = object_new(guid);
						break;
					case OBJECT_CORPSE:
						object = object_new(guid);
						break;
					default:
						LOG_ERROR("unknown object type: %d", object_type);
						return false;
				}
				if (!object)
				{
					LOG_ERROR("failed to create object");
					return false;
				}
				if (!object_read_update_data(object, packet)
				 || !object_read_fields(object, packet))
				{
					LOG_ERROR("failed to read update data");
					return false;
				}
				break;
			}
			case OBJECT_CREATE2:
			{
				uint64_t guid;
				if (!net_read_guid(packet, &guid))
					return false;
				if (wow_get_object(guid))
				{
					LOG_ERROR("OBJECT_CREATE2 already existing object: %" PRIu64, guid);
					return false;
				}
				uint8_t object_type;
				if (!net_read_u8(packet, &object_type))
					return false;
				struct object *object;
				switch ((enum object_type)object_type)
				{
					case OBJECT_OBJECT:
						object = object_new(guid);
						break;
					case OBJECT_ITEM:
						object = (struct object*)item_new(guid);
						break;
					case OBJECT_CONTAINER:
						object = (struct object*)container_new(guid);
						break;
					case OBJECT_UNIT:
						object = (struct object*)creature_new(guid);
						break;
					case OBJECT_PLAYER:
						object = (struct object*)player_new(guid);
						break;
					case OBJECT_GAMEOBJECT:
						object = (struct object*)gameobj_new(guid);
						break;
					case OBJECT_DYNAMICOBJECT:
						object = (struct object*)dynobj_new(guid);
						break;
					case OBJECT_CORPSE:
						object = (struct object*)corpse_new(guid);
						break;
					default:
						LOG_ERROR("unknown object type: %d", object_type);
						return false;
				}
				if (!object)
				{
					LOG_ERROR("failed to create object");
					return false;
				}
				if (!object_read_update_data(object, packet)
				 || !object_read_fields(object, packet))
				{
					LOG_ERROR("failed to read update data");
					return false;
				}
				break;
			}
			case OBJECT_UPDATE_OUT_OF_RANGE:
			{
				LOG_ERROR("unhandled OBJECT_UPDATE_OUT_OF_RANGE");
				break;
			}
			case OBJECT_UPDATE_IN_RANGE:
			{
				LOG_ERROR("unhandled OBJECT_UPDATE_IN_RANGE");
				break;
			}
			default:
			{
				LOG_ERROR("unknown update type: %d", update_type);
				return false;
			}
		}
	}
	return true;
}

bool net_smsg_destroy_object(struct net_packet_reader *packet)
{
	uint64_t guid;
	if (!net_read_u64(packet, &guid))
		return false;
	struct object *object = wow_get_object(guid);
	if (object)
		object_delete(object);
	else
		LOG_ERROR("deleting unexisting object guid");
	return true;
}
