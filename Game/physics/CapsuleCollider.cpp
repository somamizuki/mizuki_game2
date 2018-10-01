/*!
 * @brief	カプセルコライダー。
 */

#include "stdafx.h"
#include "Physics/CapsuleCollider.h"



/*!
	* @brief	デストラクタ。
	*/
CapsuleCollider::~CapsuleCollider()
{
	delete shape;
}
