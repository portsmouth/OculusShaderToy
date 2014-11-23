
#include "Player.h"
#include <Kernel/OVR_Alg.h>

#include <algorithm>

Player::Player() :
	UserEyeHeight(1.76f - 0.15f),        // 1.76 meters height (ave US male, Wikipedia), less 15 centimeters (TomF's top-of-head-to-eye distance).
	//BodyPos(1.0f, 1.76f - 0.15f, 1.0f),
	BodyPos(2.0f, 3.5f, 2.0f),
	BodyYaw(YawInitial)
{
	MoveForward = MoveBack = MoveLeft = MoveRight = MoveUp = MoveDown = 0;
	GamepadMove = OVR::Vector3f(0);
	GamepadRotate = OVR::Vector3f(0);

	m_velocity = OVR::Vector3f(0);

	bMotionRelativeToBody = false;
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
	OVR::Vector3f moveDir;
	if (MoveForward || MoveBack || MoveLeft || MoveRight || MoveUp || MoveDown)
	{
		if (MoveForward)
		{
			moveDir += ForwardVector;
		}
		else if (MoveBack)
		{
			moveDir -= ForwardVector;
		}
		if (MoveRight)
		{
			moveDir += RightVector;
		}
		else if (MoveLeft)
		{
			moveDir -= RightVector;
		}
		if (MoveUp)
		{
			moveDir += UpVector;
		}
		else if (MoveDown)
		{
			moveDir -= UpVector;
		}
	}
	else if (GamepadMove.LengthSq() > 0)
	{
		moveDir = GamepadMove;
	}

	moveDir = GetOrientation(bMotionRelativeToBody).Rotate(moveDir);
	moveDir.Normalize();

	OVR::Vector3f dPos = m_velocity * dt;

	OVR::Vector3f newVelocity = m_velocity + moveDir*(MoveAccel*float(dt));
	if (newVelocity.Length()<MaxMoveSpeed) m_velocity=newVelocity;

	float fracSpeed = std::min(m_velocity.Length()/MaxMoveSpeed, 1.f);
	m_velocity *= std::min(0.99, exp(-Damping*(1.0f-fracSpeed)));

	BodyPos += dPos;
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

