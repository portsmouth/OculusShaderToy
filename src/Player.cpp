
#include "Player.h"
#include <Kernel/OVR_Alg.h>

Player::Player() :
	UserEyeHeight(1.76f - 0.15f),        // 1.76 meters height (ave US male, Wikipedia), less 15 centimeters (TomF's top-of-head-to-eye distance).
	//BodyPos(1.0f, 1.76f - 0.15f, 1.0f),
	BodyPos(2.0f, 3.5f, 2.0f),
	BodyYaw(YawInitial)
{
	MoveForward = MoveBack = MoveLeft = MoveRight = MoveUp = MoveDown = 0;
	GamepadMove = OVR::Vector3f(0);
	GamepadRotate = OVR::Vector3f(0);
}

Player::~Player()
{
}

OVR::Vector3f Player::GetPosition()
{
	return BodyPos + OVR::Quatf(OVR::Vector3f(0,1,0), BodyYaw.Get()).Rotate(HeadPose.Translation);
}

OVR::Quatf Player::GetOrientation(bool baseOnly)
{
	OVR::Quatf baseQ = OVR::Quatf(OVR::Vector3f(0,1,0), BodyYaw.Get());
	return baseOnly ? baseQ : baseQ * HeadPose.Rotation;
}

OVR::Posef Player::VirtualWorldTransformfromRealPose(const OVR::Posef &sensorHeadPose)
{
	OVR::Quatf baseQ = OVR::Quatf(OVR::Vector3f(0,1,0), BodyYaw.Get());

	return OVR::Posef(baseQ * sensorHeadPose.Rotation,
				 BodyPos + baseQ.Rotate(sensorHeadPose.Translation));
}

void Player::HandleMovement(double dt, bool shiftDown)
{
	// Handle keyboard movement.
	// This translates BasePos based on the orientation and keys pressed.
	// Note that Pitch and Roll do not affect movement (they only affect view).
	OVR::Vector3f controllerMove;
	if (MoveForward || MoveBack || MoveLeft || MoveRight || MoveUp || MoveDown)
	{
		if (MoveForward)
		{
			controllerMove += ForwardVector;
		}
		else if (MoveBack)
		{
			controllerMove -= ForwardVector;
		}
		if (MoveRight)
		{
			controllerMove += RightVector;
		}
		else if (MoveLeft)
		{
			controllerMove -= RightVector;
		}
		if (MoveUp)
		{
			controllerMove += UpVector;
		}
		else if (MoveDown)
		{
			controllerMove -= UpVector;
		}
	}
	else if (GamepadMove.LengthSq() > 0)
	{
		controllerMove = GamepadMove;
	}
	controllerMove = GetOrientation(bMotionRelativeToBody).Rotate(controllerMove);
	//controllerMove.y = 0; // Project to the horizontal plane
	if (controllerMove.LengthSq() > 0)
	{
		// Normalize vector so we don't move faster diagonally.
		controllerMove.Normalize();
		controllerMove *= OVR::Alg::Min<float>(MoveSpeed * (float)dt * (shiftDown ? 3.0f : 1.0f), 1.0f);
	}

	// Compute total move direction vector and move length
	OVR::Vector3f orientationVector = controllerMove;
	float moveLength = orientationVector.Length();
	if (moveLength > 0)
		orientationVector.Normalize();

	orientationVector *= moveLength;
	BodyPos += orientationVector;
}


// Handle directional movement. Returns 'true' if movement was processed.
bool Player::HandleMoveKey(OVR::KeyCode key, bool down)
{
	switch(key)
	{
		// Handle player movement keys.
		// We just update movement state here, while the actual translation is done in the main loop
		// based on time.
		case OVR::Key_W:     MoveForward = down ? (MoveForward | 1) : (MoveForward & ~1); return true;
		case OVR::Key_S:     MoveBack    = down ? (MoveBack    | 1) : (MoveBack    & ~1); return true;
		case OVR::Key_A:     MoveLeft    = down ? (MoveLeft    | 1) : (MoveLeft    & ~1); return true;
		case OVR::Key_D:     MoveRight   = down ? (MoveRight   | 1) : (MoveRight   & ~1); return true;
		case OVR::Key_Up:    MoveUp      = down ? (MoveUp      | 2) : (MoveUp      & ~2); return true;
		case OVR::Key_Down:  MoveDown    = down ? (MoveDown    | 2) : (MoveDown    & ~2); return true;
		case OVR::Key_Left:  MoveLeft    = down ? (MoveLeft    | 2) : (MoveLeft    & ~2); return true;
		case OVR::Key_Right: MoveRight   = down ? (MoveRight   | 2) : (MoveRight   & ~2); return true;
		default: return false;
	}
}

