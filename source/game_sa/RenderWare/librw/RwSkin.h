#pragma once

//! Native `RpSkin`.
//!
//! `RenderWare/rw/rpSkin.h` only forward-declares `RpSkin`: the retail object lived inside
//! `rpskin` and the game never touches its fields. This is the object `RpSkinCreate` and the DFF
//! skin reader build, and what the GL3 ped pipeline consumes through the `RpSkin*` API.
//!
//! The on-disk skin chunk (`rwID_SKINPLUGIN`, 0x116) of a San Andreas DFF is:
//!
//!     u8   numBones
//!     u8   numUsedBones
//!     u8   maxNumWeightsForVertex
//!     u8   pad
//!     u8   usedBones[numUsedBones]             // bone indices
//!     u8   vertexBoneIndices[numVertices * 4]  // four packed bone indices per vertex
//!     f32  vertexBoneWeights[numVertices * 4]  // four floats per vertex, w0..w3
//!     RwMatrix skinToBoneMatrices[numBones]
//!     i32  boneLimit, numMeshes, rleSize       // split-data header
//!     ...  remap indices / RLE when numMeshes != 0
//!
//! Verified against the skinned DFFs of `models/player.img`: this order yields unit-length bone
//! matrices and weights in [0, 1] (the matrix-first order used by older RW versions yields
//! garbage). It matches librw's `readSkin` for library versions >= 0x34000.
//!
//! The vertex indices are *packed* (`uint8[4]`) although `RpSkinGetVertexBoneIndices` returns
//! `const RwUInt32*`: that is how the retail API exposes them and how the GL3 ped pipeline binds
//! the attribute (`4 x GL_UNSIGNED_BYTE`, stride `sizeof(RwUInt32)`).

#ifdef NOTSA_NATIVE

#include "RwFakerw.h"

struct RpSkin {
    RwUInt32 numVertices;            //!< Length of `vertexBoneIndices` / `vertexBoneWeights`.
    RwUInt32 numBones;               //!< Length of `skinToBoneMatrices`.
    RwUInt32 numUsedBones;           //!< Number of set entries in `usedBones`.
    RwUInt8  maxNumWeightsForVertex; //!< 1..4, as stored in the skin chunk.

    bool*            usedBones;          //!< One flag per bone (`numBones`): referenced by a vertex.
    RwMatrix*        skinToBoneMatrices; //!< `numBones` skin-to-bone (inverse bind) matrices.
    RwUInt32*        vertexBoneIndices;  //!< `numVertices`, four packed `RwUInt8` each.
    RwMatrixWeights* vertexBoneWeights;  //!< `numVertices`, weights sorted w0..w3.

    //! Split/RLE data built by `_rpSkinSplitDataCreate`. The GL3 ped pipeline renders the skin
    //! in one pass, so this stays null unless a caller explicitly asks for the split form.
    RwUInt32 boneLimit;
    RwUInt32 numMeshes;
    RwUInt32 rleSize;
    RwUInt8* splitData;              //!< `numBones + 2 * (numMeshes + rleSize)` bytes, or null.
};

//! Native-only accessors. Retail `rpSkin.h` has no equivalent for these; nothing in the game
//! calls them yet, they exist so the native skin is fully queryable (e.g. for debugging).
RwUInt32 RpSkinGetNumUsedBones(RpSkin* skin);
bool*    RpSkinGetUsedBones(RpSkin* skin);
RwUInt8  RpSkinGetMaxNumWeightsForVertex(RpSkin* skin);
RpSkin*  RpSkinSetSkinToBoneMatrices(RpSkin* skin, const RwMatrix* matrices);

#endif // NOTSA_NATIVE
