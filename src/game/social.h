#ifndef SOCIAL_H
#define SOCIAL_H

#include <jks/array.h>

#include <stdbool.h>
#include <stdint.h>

struct social_friend
{
	uint64_t guid;
	uint8_t level;
	uint8_t class_type;
	uint8_t status;
	uint32_t zone;
	char *note;
};

struct social_ignore
{
	uint64_t guid;
};

struct social
{
	struct jks_array friends; /* social_friend_t */
	struct jks_array ignores; /* social_ignore_t */
	uint32_t selected_friend;
	uint32_t selected_ignore;
	bool loaded;
};

struct social *social_new(void);
void social_delete(struct social *social);
bool social_net_load(struct social *social);
bool social_add_friend(struct social *social, struct social_friend *friend_s);
bool social_remove_friend(struct social *social, uint64_t guid);
bool social_add_ignore(struct social *social, struct social_ignore *ignore);
bool social_remove_ignore(struct social *social, uint64_t guid);

#endif
