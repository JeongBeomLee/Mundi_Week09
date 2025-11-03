PlayerRotationUtility = {};

function PlayerRotationUtility.New()
    return {RotationRecorded=FVector(0.0, 0.0, 0.0)};
end 

function PlayerRotationUtility.Initilaize(PRU, InRotation)
    PRU.RotationRecorded = InRotation;
end

function PlayerRotationUtility.IsUpdated(PRU, CurrentRotation)
    -- FVector의 각 요소를 개별적으로 비교
    if (PRU.RotationRecorded.X ~= CurrentRotation.X or
        PRU.RotationRecorded.Y ~= CurrentRotation.Y or
        PRU.RotationRecorded.Z ~= CurrentRotation.Z) then
        PRU.RotationRecorded = CurrentRotation;
        return true;
    end
    return false;
end

return PlayerRotationUtility;
