import mongoose,{Schema,Document} from "mongoose";

export interface IUser extends Document{
    name: string;
    surname: string;
    email: string;
    gymMembershipStarts: Date;
    gymMembershipEnds: Date;
    coffeePoints: number;
    card_UID: string;
}

const UserSchema = new Schema({
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
        match: [/^\S+@\S+\.\S+$/,"incorrect email "],
    },
    gymMembershipStarts: {
        type:Date,
        required: true,
        default: Date.now
    },
    gymMembershipEnds: {
        type: Date,
        required: true,
        default: () => {
            const date = new Date();
            date.setMonth(date.getMonth() + 3);
            return date;
        }
    },
    coffeePoints: {
        type: Number,
        required: true,
        default: 1000
    },
    card_UID: {
        type: String,
        required: true,
        unique: true
    }

})

export const User = mongoose.model<IUser>("User", UserSchema);

