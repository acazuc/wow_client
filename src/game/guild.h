#ifndef GUILD_H
#define GUILD_H

struct guild_rank
{
};

struct guild_member
{
	uint64_t guid;
	char *name;
};

struct guild
{
	struct jks_array members; /* struct guild_member */
	char *name;
	char *motd;
	uint64_t guid;
};

#endif
