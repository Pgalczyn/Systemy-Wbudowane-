import mongoose, { Schema, Types } from "mongoose";

export interface IGymSession {
    user: Types.UUID;
    enterTime: Date;
    exitTime?: Date;
    isAtTheGym: boolean;
}

const gymSessionSchema = new Schema<IGymSession>({
    user: {
        type: Schema.Types.UUID,
        ref: "User",
        required: true
    },
    enterTime: {
        type: Date,
        required: true,
        default: Date.now
    },
    exitTime: {
        type: Date,
        required: false,
    },
    isAtTheGym: {
        type: Boolean,
        required: true,
        default: true
    },
})

export const GymSession = mongoose.model<IGymSession>("GymSession", gymSessionSchema);