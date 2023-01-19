#include "net/packets.h"
#include "net/packet.h"

#include "obj/worldobj.h"
#include "obj/object.h"

#include "wow.h"
#include "log.h"

#include <inttypes.h>

bool net_smsg_move(struct net_packet_reader *packet)
{
	uint64_t guid;
	if (!net_read_guid(packet, &guid))
		return false;
	struct object *object = wow_get_object(guid);
	if (!object)
	{
		LOG_ERROR("unknown guid %" PRIu64, guid);
		return false;
	}
	struct worldobj *worldobj = object_as_worldobj(object);
	if (!worldobj)
	{
		LOG_ERROR("object isn't worldobj");
		return false;
	}
	if (!worldobj_read_movement_data(worldobj, packet))
		return false;
	return true;
}
