#include "StdInc.h"

#include "rtdict.h"

RtDictSchema* RtDictSchemaInit(RtDictSchema* schema) {
    return plugin::CallAndReturn<RtDictSchema*, 0x7CED40, RtDictSchema*>(schema);
}

RwBool RtDictSchemaDestruct(RtDictSchema* schema) {
    return plugin::CallAndReturn<RwBool, 0x7CED70, RtDictSchema*>(schema);
}

RtDict* RtDictSchemaStreamReadDict(RtDictSchema* schema, RwStream* stream) {
    return plugin::CallAndReturn<RtDict*, 0x7CF240, RtDictSchema*, RwStream*>(schema, stream);
}

RtDict* RtDictSchemaCreateDict(RtDictSchema* schema) {
    return plugin::CallAndReturn<RtDict*, 0x7CED90, RtDictSchema*>(schema);
}

RtDictSchema* RtDictSchemaAddDict(RtDictSchema* schema, RtDict* dict) {
    return plugin::CallAndReturn<RtDictSchema*, 0x7CEE50, RtDictSchema*, RtDict*>(schema, dict);
}

RtDictSchema* RtDictSchemaRemoveDict(RtDictSchema* schema, RtDict* dict) {
    return plugin::CallAndReturn<RtDictSchema*, 0x7CEE80, RtDictSchema*, RtDict*>(schema, dict);
}

RwBool RtDictSchemaForAllDictionaries(RtDictSchema* schema, RtDictCallBack* callback, RtDictEntryType data) {
    return plugin::CallAndReturn<RwBool, 0x7CF5B0, RtDictSchema*, RtDictCallBack*, RtDictEntryType>(schema, callback, data);
}

RtDict* RtDictSchemaGetCurrentDict(RtDictSchema* schema) {
    return plugin::CallAndReturn<RtDict*, 0x7CEEE0, RtDictSchema*>(schema);
}

RtDictSchema* RtDictSchemaSetCurrentDict(RtDictSchema* schema, RtDict* dict) {
    return plugin::CallAndReturn<RtDictSchema*, 0x7CEEF0, RtDictSchema*, RtDict*>(schema, dict);
}

RwBool RtDictDestroy(RtDict* dictionary) {
    return plugin::CallAndReturn<RwBool, 0x7CF130, RtDict*>(dictionary);
}

RtDictEntryType RtDictAddEntry(RtDict* dictionary, RtDictEntryType entry) {
    return plugin::CallAndReturn<RtDictEntryType, 0x7CEFB0, RtDict*, RtDictEntryType>(dictionary, entry);
}

RtDictEntryType RtDictFindNamedEntry(RtDict* dictionary, const RwChar* name) {
    return plugin::CallAndReturn<RtDictEntryType, 0x7CEFE0, RtDict*, const RwChar*>(dictionary, name);
}

const RtDict* RtDictForAllEntries(const RtDict* dictionary, RtDictEntryCallBack* callBack, RtDictEntryType data) {
    return plugin::CallAndReturn<const RtDict*, 0x7CF060, const RtDict*, RtDictEntryCallBack*, RtDictEntryType>(dictionary, callBack, data);
}

RtDict* RtDictRemoveEntry(RtDict* dictionary, RtDictEntryType entry) {
    return plugin::CallAndReturn<RtDict*, 0x7CF0C0, RtDict*, RtDictEntryType>(dictionary, entry);
}

const RtDict* RtDictStreamWrite(const RtDict* dictionary, RwStream* stream) {
    return plugin::CallAndReturn<const RtDict*, 0x7CF490, const RtDict*, RwStream*>(dictionary, stream);
}

RwUInt32 RtDictStreamGetSize(const RtDict* dictionary) {
    return plugin::CallAndReturn<RwUInt32, 0x7CF1F0, const RtDict*>(dictionary);
}

RtDict* _rtDictSchemaInitDict(RtDictSchema* schema, RtDict* dictionary) {
    return plugin::CallAndReturn<RtDict*, 0x7CEF00, RtDictSchema*, RtDict*>(schema, dictionary);
}

RwBool _rtDictDestruct(RtDict* dictionary) {
    return plugin::CallAndReturn<RwBool, 0x7CEF60, RtDict*>(dictionary);
}
