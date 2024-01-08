#include "Blob.hxx"

void SBlob::Update(float DeltaTime) {
    AnimationAlpha += DeltaTime * 3.3f;
    if (AnimationAlpha >= 1.0f) {
        AnimationAlpha = 1.0f;
    }
    switch (AnimationType) {
        case EPlayerAnimationType::Turn:
            EyeForwardCurrent = UVec3::Mix(EyeForwardFrom, EyeForwardTarget, AnimationAlpha);
            break;
        case EPlayerAnimationType::Walk:
            EyePositionCurrent = UVec3::Mix(EyePositionFrom, EyePositionTarget, AnimationAlpha);
            break;
        case EPlayerAnimationType::Bump:
            EyePositionCurrent = UVec3::Mix(EyePositionTarget, EyePositionFrom,
                                            std::sin(AnimationAlpha * Math::PI) * 0.25f);
            break;
        case EPlayerAnimationType::Idle:
        default:
            break;
    }
    if (AnimationAlpha >= 1.0f) {
        AnimationType = EPlayerAnimationType::Idle;
    }
}

SBlob::SBlob() {
    EyePositionCurrent = EyePositionTarget = {(float) Coords.X, EyeHeight, (float) Coords.Y};
    ApplyDirection(true);
}

void SBlob::Turn(bool bRight) {
    if (AnimationType != EPlayerAnimationType::Idle)
        return;
    if (!bRight) {
        Direction.CycleCCW();
        ApplyDirection(false);
    } else {
        Direction.CycleCW();
        ApplyDirection(false);
    }
}

void SBlob::ApplyDirection(bool bImmediate) {
    EyeForwardTarget = {0.0f, 0.0f, 0.0f};
    auto DirectionVector = Direction.GetVector<float>();
    EyeForwardTarget.X += DirectionVector.X;
    EyeForwardTarget.Z += DirectionVector.Y;

    if (bImmediate) {
        AnimationType = EPlayerAnimationType::Idle;
        AnimationAlpha = 1.0f;
        EyeForwardCurrent = EyeForwardTarget;
    } else {
        AnimationType = EPlayerAnimationType::Turn;
        AnimationAlpha = 0.0f;
        EyeForwardFrom = EyeForwardCurrent;
    }
}

void SBlob::Step(UVec2Int DirectionVector) {
    if (AnimationType != EPlayerAnimationType::Idle)
        return;
    AnimationType = EPlayerAnimationType::Walk;
    AnimationAlpha = 0.0f;
    EyePositionFrom = EyePositionCurrent;
    EyePositionTarget += UVec3{(float) DirectionVector.X, 0.0f, (float) DirectionVector.Y};
    Coords += DirectionVector;
}

void SBlob::BumpIntoWall() {
    if (AnimationType != EPlayerAnimationType::Idle)
        return;
    AnimationType = EPlayerAnimationType::Bump;
    AnimationAlpha = 0.0f;

    EyePositionFrom = EyePositionCurrent + (EyeForwardCurrent / 2.0f);
}
