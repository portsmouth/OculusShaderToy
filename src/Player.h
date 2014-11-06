
#pragma once

// Include the OculusVR SDK
#include "OVR.h"
#include "OVR_Kernel.h"
#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"
#include <Kernel/OVR_Math.h>
#include <Kernel/OVR_Alg.h>
#include "Kernel/OVR_KeyCodes.h"


//-------------------------------------------------------------------------------------
// The RHS coordinate system is assumed.
const OVR::Vector3f	RightVector(1.0f, 0.0f, 0.0f);
const OVR::Vector3f	UpVector(0.0f, 1.0f, 0.0f);
const OVR::Vector3f	ForwardVector(0.0f, 0.0f, -1.0f); // -1 because HMD looks along -Z at identity orientation

const float		YawInitial	= 0.0f;
const float		Sensitivity	= 0.3f; // low sensitivity to ease people into it gently.
const float		MoveSpeed	= 1.0f; // m/s


// Player class describes position and movement state of the player in the 3D world.
class Player
{
public:

	float				UserEyeHeight;

	// Where the avatar coordinate system (and body) is positioned and oriented in the virtual world
	// Modified by gamepad/mouse input
	OVR::Vector3f			BodyPos;
	OVR::Anglef				BodyYaw;

	// Where the player head is positioned and oriented in the real world
	OVR::Posef          HeadPose;

	// Where the avatar head is positioned and oriented in the virtual world
	OVR::Vector3f            GetPosition();
	OVR::Quatf               GetOrientation(bool baseOnly = false);

	// Returns virtual world position based on a real world head pose.
	// Allows predicting eyes separately based on scanout time.
	OVR::Posef          VirtualWorldTransformfromRealPose(const OVR::Posef &sensorHeadPose);

	// Handle directional movement. Returns 'true' if movement was processed.
	bool                HandleMoveKey(OVR::KeyCode key, bool down);

	// Movement state; different bits may be set based on the state of keys.
	uint8_t             MoveForward;
	uint8_t             MoveBack;
	uint8_t             MoveLeft;
	uint8_t             MoveRight;
	uint8_t             MoveUp;
	uint8_t             MoveDown;
	OVR::Vector3f       GamepadMove, GamepadRotate;
	bool                bMotionRelativeToBody;

	Player();
	~Player();

	void HandleMovement(double dt, bool shiftDown);

};
