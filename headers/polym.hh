#pragma once
#include <iosfwd>

class NType; class NPolym;

void register_polym(NPolym* np, std::ostream& dout);
int getPolymOf(const NType* in);
