#include "performance.h"

#include "log.h"

#include <inttypes.h>
#include <string.h>
#include <limits.h>

struct performance_report g_performances[PERFORMANCE_LAST];

#ifdef WITH_PERFORMANCE
static const char *strings[PERFORMANCE_LAST] =
{
	"SKYBOX_RENDER",
	"ADT_CULL",
	"ADT_AABB_RENDER",
	"ADT_OBJECTS_CULL",
	"MCNK_CULL",
	"MCNK_RENDER",
	"MCNK_RENDER_DATA",
	"MCNK_RENDER_BIND",
	"MCNK_RENDER_DRAW",
	"MCNK_AABB_RENDER",
	"MCLQ_CULL",
	"MCLQ_RENDER",
	"MCLQ_AABB_RENDER",
	"WDL_CULL",
	"WDL_RENDER",
	"WDL_AABB_RENDER",
	"M2_CULL",
	"M2_RENDER",
	"M2_RENDER_DATA",
	"M2_RENDER_BIND",
	"M2_RENDER_DRAW",
	"M2_AABB_RENDER",
	"M2_BONES_RENDER",
	"M2_LIGHTS_RENDER",
	"M2_PARTICLES_RENDER",
	"M2_COLLISIONS_RENDER",
	"WMO_CULL",
	"WMO_RENDER",
	"WMO_RENDER_DATA",
	"WMO_RENDER_BIND",
	"WMO_RENDER_DRAW",
	"WMO_AABB_RENDER",
	"WMO_PORTALS_CULL",
	"WMO_LIGHTS_RENDER",
	"WMO_PORTALS_RENDER",
	"WMO_LIQUIDS_RENDER",
	"WMO_COLLISIONS_RENDER",
	"COLLISIONS",
	"COLLISIONS_RENDER",
};
#endif

void performance_reset(void)
{
#ifdef WITH_PERFORMANCE
	for (size_t i = 0; i < sizeof(g_performances) / sizeof(*g_performances); ++i)
	{
		struct performance_report *report = &g_performances[i];
		report->samples = 0;
		report->sum = 0;
		report->min = UINT_MAX;
		report->max = 0;
	}
#endif
}

void performance_dump(void)
{
#ifdef WITH_PERFORMANCE
	LOG_INFO("%20s | %7s | %6s | %6s | %8s | %7s", "category", "samples", "min", "max", "average", "total");
	LOG_INFO("---------------------+---------+--------+--------+----------+--------");
	for (size_t i = 0; i < sizeof(g_performances) / sizeof(*g_performances); ++i)
	{
		uint64_t samples = g_performances[i].samples;
		uint64_t min = samples ? g_performances[i].min / 1000 : 0;
		uint64_t max = samples ? g_performances[i].max / 1000 : 0;
		float avg = samples ? g_performances[i].samples : 1;
		avg = g_performances[i].sum / avg / 1000;
		uint64_t sum = samples ? g_performances[i].sum / 1000 : 0;
		LOG_INFO("%20s | %7" PRIu64 " | %6" PRIu64 " | %6" PRIu64 " | %8.2lf | %7" PRIu64, strings[i], samples, min, max, avg, sum);
	}
	LOG_INFO(" ");
#endif
}

void performance_add(enum performance_category category, uint64_t time)
{
#ifdef WITH_PERFORMANCE
	g_performances[category].samples++;
	g_performances[category].sum += time;
	if (time < g_performances[category].min)
		g_performances[category].min = time;
	if (time > g_performances[category].max)
		g_performances[category].max = time;
#else
	(void)category;
	(void)time;
#endif
}
