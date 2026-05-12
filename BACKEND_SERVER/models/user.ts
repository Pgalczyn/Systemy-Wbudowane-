import mongoose,{Schema,Document} from "mongoose";

export interface IUser extends Document{
    name: string;
    surname: string;
    email: string;
    gymMembershipStarts: Date;
    gymMembershipEnds: Date;
    coffeePoints: number;
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
    },
    gymMembershipEnds: {
        type:Date,
        required: true
    },
    coffeePoints: {
        type: Number,
        required: true,
    }

})

export const User = mongoose.model<IUser>("User", UserSchema);

