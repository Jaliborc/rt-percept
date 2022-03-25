/***************************************************************************
 # Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#pragma once

namespace Falcor
{
    /** StreamOutputState state
    */
    class dlldecl StreamOutputState : public std::enable_shared_from_this<StreamOutputState>
    {
    public:
        using SharedPtr = std::shared_ptr<StreamOutputState>;
        using SharedConstPtr = std::shared_ptr<const StreamOutputState>;

        /** Descriptor used to create new blend-state
        */
        class dlldecl Desc
        {
        public:
            Desc();
            friend class StreamOutputState;
        };

        ~StreamOutputState();

        /** Create a new blend state object.
            \param[in] Desc Blend state descriptor.
            \return A new object, or throws an exception if creation failed.
        */
        static StreamOutputState::SharedPtr create(const Desc& desc);

        /** Get the API handle
        */
        const StreamOutputState& getApiHandle() const;

        const std::vector<unsigned int>& getStrides() const { return strides; }
        const std::vector< D3D12_SO_DECLARATION_ENTRY>& getDeclarations() const { return declarations; }

        void setStrides(const std::vector<unsigned int> str) { strides = str; }
        void setDeclarations(const std::vector< D3D12_SO_DECLARATION_ENTRY> entries) { declarations = entries; }

        unsigned int getRasterizedStream() const { return rasterizedStream; }
        void setRasterizedStream(unsigned int r) { rasterizedStream = r; }

    private:

        std::vector<unsigned int> strides;
        std::vector<D3D12_SO_DECLARATION_ENTRY> declarations;
        unsigned int rasterizedStream = 0;

        StreamOutputState(const Desc& Desc) : mDesc(Desc) {}
        const Desc mDesc;
        StreamOutputStateHandle mApiHandle;
    };
}
