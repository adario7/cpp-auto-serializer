#pragma once

#include <node.hh>

void init_types();
void add_alias(const segment_t pos, const std::string& name, const std::string& real);

void serialize_field(const std::string&, const NType&, std::ostream& o);
void deserialize_field(const std::string&, const NType&, std::ostream& o);
void deserialize_value(const std::string& fname, const NType& t, std::ostream& o);
