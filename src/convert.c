#include "src/convert.h"

struct ConvertEngine {
	ConvertBuildf build;
	ConvertRunf run;
};

const struct ConvertEngine kEngines[][2] = {
	{{Convert1fBuild, Convert1fRun}, {Convert1rBuild, Convert1rRun}}};

int ConverterBuild(struct Converter *c, Handle data, Size datasz,
                   ConvertDirection direction, OSErr *errp)
{
	int engine, r;
	const struct ConvertEngine *funcs;
	Handle out;

	if (datasz == 0) {
		return kErrorBadData;
	}
	engine = (UInt8) * *data - 1;
	if (engine < 0 || (int)(sizeof(kEngines) / sizeof(*kEngines)) <= engine) {
		/* Invalid engine. */
		return kErrorBadData;
	}
	funcs = &kEngines[engine][direction];
	if (funcs->build == NULL || funcs->run == NULL) {
		/* Invalid engine. */
		return kErrorBadData;
	}
	r = funcs->build(&out, data, datasz, errp);
	if (r != 0) {
		return r;
	}
	c->data = out;
	c->run = funcs->run;
	return 0;
}
