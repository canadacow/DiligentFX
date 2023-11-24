/*
 *  Copyright 2023 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "HnMaterialNetwork.hpp"
#include "HnTextureRegistry.hpp"

#include "pxr/imaging/hd/material.h"

#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../../DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "../../../DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../../DiligentCore/Common/interface/BasicMath.hpp"

namespace Diligent
{

class PBR_Renderer;
class USD_Renderer;

namespace GLTF
{
class ResourceManager;
}

namespace HLSL
{
#include "Shaders/PBR/public/PBR_Structures.fxh"
}

namespace USD
{

class HnTextureRegistry;

/// Hydra material implementation in Hydrogent.
class HnMaterial final : public pxr::HdMaterial
{
public:
    static HnMaterial* Create(const pxr::SdfPath& id);
    static HnMaterial* CreateFallback(HnTextureRegistry& TexRegistry, const USD_Renderer& UsdRenderer);

    ~HnMaterial();

    // Synchronizes state from the delegate to this object.
    virtual void Sync(pxr::HdSceneDelegate* SceneDelegate,
                      pxr::HdRenderParam*   RenderParam,
                      pxr::HdDirtyBits*     DirtyBits) override final;

    // Returns the minimal set of dirty bits to place in the
    // change tracker for use in the first sync of this prim.
    virtual pxr::HdDirtyBits GetInitialDirtyBitsMask() const override final;

    static RefCntAutoPtr<IObject> CreateSRBCache();

    void UpdateSRB(IObject*      pSRBCache,
                   PBR_Renderer& PbrRenderer,
                   IBuffer*      pFrameAttribs,
                   Uint32        AtlasVersion);

    IShaderResourceBinding* GetSRB() const { return m_SRB; }
    IShaderResourceBinding* GetSRB(Uint32 PrimitiveAttribsOffset) const
    {
        if (m_PrimitiveAttribsVar != nullptr)
            m_PrimitiveAttribsVar->SetBufferOffset(PrimitiveAttribsOffset);
        return m_SRB;
    }

    const HLSL::PBRMaterialBasicAttribs&   GetBasicShaderAttribs() const { return m_BasicShaderAttribs; }
    const HLSL::PBRMaterialTextureAttribs* GetShaderTextureAttribs() const { return m_ShaderTextureAttribs.get(); }
    Uint32                                 GetNumShaderTextureAttribs() const { return m_NumShaderTextureAttribs; }

    /// Texture coordinate set info
    struct TextureCoordinateSetInfo
    {
        /// Texture coordinate set primvar name (e.g. "st")
        pxr::TfToken PrimVarName;
    };
    const auto& GetTextureCoordinateSets() const { return m_TexCoords; }

    const pxr::TfToken& GetTag() const { return m_Network.GetTag(); }

private:
    HnMaterial(pxr::SdfPath const& id);

    // Special constructor for fallback material.
    //
    // \remarks     Sync() is not called on fallback material,
    //  	        but we need to initialize default textures,
    //  	        so we have to use this special constructor.
    HnMaterial(HnTextureRegistry& TexRegistry, const USD_Renderer& UsdRenderer);

    RefCntAutoPtr<ITexture> GetTexture(const pxr::TfToken& Name) const;

    using TexNameToCoordSetMapType = std::unordered_map<pxr::TfToken, size_t, pxr::TfToken::HashFunctor>;
    void AllocateTextures(HnTextureRegistry& TexRegistry, TexNameToCoordSetMapType& TexNameToCoordSetMap);

    HnTextureRegistry::TextureHandleSharedPtr GetDefaultTexture(HnTextureRegistry& TexRegistry, const pxr::TfToken& Name);

    void InitTextureAttribs(HnTextureRegistry& TexRegistry, const USD_Renderer& UsdRenderer, const TexNameToCoordSetMapType& TexNameToCoordSetMap);

private:
    HnMaterialNetwork m_Network;

    std::unordered_map<pxr::TfToken, HnTextureRegistry::TextureHandleSharedPtr, pxr::TfToken::HashFunctor> m_Textures;

    RefCntAutoPtr<IShaderResourceBinding> m_SRB;
    IShaderResourceVariable*              m_PrimitiveAttribsVar = nullptr; // cbPrimitiveAttribs

    HLSL::PBRMaterialBasicAttribs                      m_BasicShaderAttribs{};
    std::unique_ptr<HLSL::PBRMaterialTextureAttribs[]> m_ShaderTextureAttribs;
    Uint32                                             m_NumShaderTextureAttribs = 0;

    std::vector<TextureCoordinateSetInfo> m_TexCoords;

    // True if there is at least one texture suballocated from the atlas
    bool m_UsesAtlas = false;

    // Current atlas version
    Uint32 m_AtlasVersion = 0;
};

} // namespace USD

} // namespace Diligent
