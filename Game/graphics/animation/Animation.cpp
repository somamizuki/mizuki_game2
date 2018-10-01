/*!
 * @brief	アニメータークラス。
 */


#include "stdafx.h"
#include "graphics/animation/Animation.h"
#include "graphics/skeleton.h"
#include "graphics/skinModel.h"

Animation::Animation()
{
}
Animation::~Animation()
{
	
}
	
void Animation::Init(SkinModel& skinModel, AnimationClip animClipList[], int numAnimClip)
{
	if (animClipList == nullptr) {
#ifdef _DEBUG
		char message[256];
		strcpy(message, "animClipListがNULLです。\n");
		OutputDebugStringA(message);
		//止める。
		std::abort();
#endif
		
	}
	m_skeleton = &skinModel.GetSkeleton();

	for (int i = 0; i < numAnimClip; i++) {
		m_animationClips.push_back(&animClipList[i]);
	}
	for (auto& ctr : m_animationPlayController) {
		ctr.Init(m_skeleton);
	}
		
	Play(0);
}
/*!
* @brief	ローカルポーズの更新。
*/
void Animation::UpdateLocalPose(float deltaTime)
{
	m_interpolateTime += deltaTime;
	if (m_interpolateTime >= 1.0f) {
		//補間完了。
		//現在の最終アニメーションコントローラへのインデックスが開始インデックスになる。
		m_startAnimationPlayController = GetLastAnimationControllerIndex();
		m_numAnimationPlayController = 1;
		m_interpolateTime = 1.0f;
	}
	//AnimationPlayController::Update関数を実行していく。
	for (int i = 0; i < m_numAnimationPlayController; i++) {
		int index = GetAnimationControllerIndex(m_startAnimationPlayController, i );
		m_animationPlayController[index].Update(deltaTime, this);
	}
}

void Animation::UpdateGlobalPose()
{
	//グローバルポーズ計算用のメモリをスタックから確保。
	int numBone = m_skeleton->GetNumBones();
	CQuaternion* qGlobalPose = (CQuaternion*)alloca(sizeof(CQuaternion) * numBone);
	CVector3* vGlobalPose = (CVector3*)alloca(sizeof(CVector3) * numBone);
	for (int i = 0; i < numBone; i++) {
		qGlobalPose[i] = CQuaternion::Identity();
		vGlobalPose[i] = CVector3::Zero();
	}
	//グローバルポーズを計算していく。
	int startIndex = m_startAnimationPlayController;
	for (int i = 0; i < m_numAnimationPlayController; i++) {
		int index = GetAnimationControllerIndex(startIndex, i);
		float intepolateRate = m_animationPlayController[index].GetInterpolateRate();
		const auto& localBoneMatrix = m_animationPlayController[index].GetBoneLocalMatrix();
		for (int boneNo = 0; boneNo < numBone; boneNo++) {
			//平行移動の補完
			CMatrix m = localBoneMatrix[boneNo];
			vGlobalPose[boneNo].Lerp(
				intepolateRate, 
				vGlobalPose[boneNo], 
				*(CVector3*)m.m[3]
			);
			//平行移動成分を削除。
			m.m[3][0] = 0.0f;
			m.m[3][1] = 0.0f;
			m.m[3][2] = 0.0f;
				
			//回転の補完
			CQuaternion qBone;
			qBone.SetRotation(m);
			qGlobalPose[boneNo].Slerp(intepolateRate, qGlobalPose[boneNo], qBone);
		}
	}
	//グローバルポーズをスケルトンに反映させていく。
	for (int boneNo = 0; boneNo < numBone; boneNo++) {
		CMatrix boneMatrix;
		boneMatrix.MakeRotationFromQuaternion(qGlobalPose[boneNo]);
		CMatrix transMat;
		transMat.MakeTranslation(vGlobalPose[boneNo]);
		boneMatrix.Mul(boneMatrix, transMat);

		m_skeleton->SetBoneLocalMatrix(
			boneNo,
			boneMatrix
		);
	}
		
	//最終アニメーション以外は補間完了していたら除去していく。
	int numAnimationPlayController = m_numAnimationPlayController;
	for (int i = 1; i < m_numAnimationPlayController; i++) {
		int index = GetAnimationControllerIndex(startIndex, i);
		if (m_animationPlayController[index].GetInterpolateRate() > 0.99999f) {
			//補間が終わっているのでアニメーションの開始位置を前にする。
			m_startAnimationPlayController = index;
			numAnimationPlayController = m_numAnimationPlayController - i;
		}
	}
	m_numAnimationPlayController = numAnimationPlayController;
}
	

	
void Animation::Update(float deltaTime)
{
	if (m_numAnimationPlayController == 0) {
		return;
	}
	//ローカルポーズの更新をやっていく。
	UpdateLocalPose(deltaTime);
		
	//グローバルポーズを計算していく。
	UpdateGlobalPose();
}
