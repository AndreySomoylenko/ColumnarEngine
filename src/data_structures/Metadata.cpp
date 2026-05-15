#include "MetaData.h"

size_t MetaData::GetColumnIndexByName(const std::string &name) const {
    return scheme.GetColumnIndexByName(name);
}