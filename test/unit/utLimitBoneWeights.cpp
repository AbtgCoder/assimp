/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/
#include "UnitTestPCH.h"

#include "PostProcessing/LimitBoneWeightsProcess.h"
#include <assimp/scene.h>

using namespace std;
using namespace Assimp;

class LimitBoneWeightsTest : public ::testing::Test {
public:
    LimitBoneWeightsTest() :
            Test(), mProcess(nullptr), mMesh(nullptr) {
        // empty
    }

protected:
    virtual void SetUp();
    virtual void TearDown();

protected:
    LimitBoneWeightsProcess *mProcess;
    aiMesh *mMesh;
};

// ------------------------------------------------------------------------------------------------
void LimitBoneWeightsTest::SetUp() {
    // construct the process
    this->mProcess = new LimitBoneWeightsProcess();

    // now need to create a nice mesh for testing purposes
    this->mMesh = new aiMesh();

    mMesh->mNumVertices = 500;
    mMesh->mVertices = new aiVector3D[500]; // uninit.
    mMesh->mNumBones = 30;
    mMesh->mBones = new aiBone *[30];
    unsigned int iCur = 0;
    for (unsigned int i = 0; i < 30; ++i) {
        aiBone *pc = mMesh->mBones[i] = new aiBone();
        pc->mNumWeights = 250;
        pc->mWeights = new aiVertexWeight[pc->mNumWeights];
        for (unsigned int qq = 0; qq < pc->mNumWeights; ++qq) {
            aiVertexWeight &v = pc->mWeights[qq];
            v.mVertexId = iCur++;
            if (500 == iCur) {
                iCur = 0;
            }
            v.mWeight = 1.0f / 15; // each vertex should occur once in two bones
        }
    }
}

// ------------------------------------------------------------------------------------------------
void LimitBoneWeightsTest::TearDown() {
    delete mMesh;
    delete mProcess;
}

// ------------------------------------------------------------------------------------------------
TEST_F(LimitBoneWeightsTest, testProcess) {
    // execute the step on the given data
    mProcess->ProcessMesh(mMesh);

    // check whether everything is ok ...
    typedef std::vector<LimitBoneWeightsProcess::Weight> VertexWeightList;
    VertexWeightList *asWeights = new VertexWeightList[mMesh->mNumVertices];

    for (unsigned int i = 0; i < mMesh->mNumVertices; ++i) {
        asWeights[i].reserve(4);
    }

    // sort back as per-vertex lists
    for (unsigned int i = 0; i < mMesh->mNumBones; ++i) {
        aiBone &pcBone = **(mMesh->mBones + i);
        for (unsigned int q = 0; q < pcBone.mNumWeights; ++q) {
            aiVertexWeight weight = pcBone.mWeights[q];
            asWeights[weight.mVertexId].emplace_back(i, weight.mWeight);
        }
    }

    // now validate the size of the lists and check whether all weights sum to 1.0f
    for (unsigned int i = 0; i < mMesh->mNumVertices; ++i) {
        EXPECT_LE(asWeights[i].size(), 4U);
        float fSum = 0.0f;
        for (VertexWeightList::const_iterator iter = asWeights[i].begin(); iter != asWeights[i].end(); ++iter) {
            fSum += (*iter).mWeight;
        }
        EXPECT_GE(fSum, 0.95F);
        EXPECT_LE(fSum, 1.04F);
    }

    // delete allocated storage
    delete[] asWeights;

    // everything seems to be OK
}
