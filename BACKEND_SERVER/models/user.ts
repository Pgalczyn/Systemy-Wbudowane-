import mongoose, { Schema, Types } from "mongoose";

export interface IUser {
    _id: Types.UUID;
    name: string;
    surname: string;
    email: string;
    membershipStart: Date;
    membershipEnd: Date;
    points: number;
    cardUid: string;
    membershipState?: "ACTIVE" | "INACTIVE";
}

const UserSchema = new Schema<IUser>({
        _id: {
        type: Schema.Types.UUID,
        required: true,
        default: () => new mongoose.Types.UUID()
    },
    name: {
        type: String,
        required: true,
        trim: true
    },
    surname: {
        type: String,
        required: true,
        trim: true
    },
    email: {
        type: String,
        required: true,
        unique: true,
        match: [/^\S+@\S+\.\S+$/, "incorrect email "],
    },
    membershipStart: {
        type: Date,
        required: true,
        default: Date.now
    },
    membershipEnd: {
        type: Date,
        required: true,
        default: () => {
            const date = new Date();
            date.setMonth(date.getMonth() + 1);
            return date;
        }
    },
    points: {
        type: Number,
        required: true,
        default: 100
    },
    cardUid: {
        type: String,
        required: true,
        unique: true
    },
    membershipState: {
        type: String,
        enum: ["ACTIVE", "INACTIVE"],
        required: true,
        default: "ACTIVE"
    }

})

export const User = mongoose.model<IUser>("User", UserSchema);

