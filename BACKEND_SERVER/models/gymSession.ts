import mongoose,{Schema,Document} from "mongoose";

export interface IgymSession  extends Document{
    user: mongoose.Types.ObjectId;
    enterDate: Date;
    exitDate: Date;
    sessionDuration: number;
    isAtTheGym:boolean;
}

const gymSessionSchema = new Schema({
    user: {
        type: Schema.Types.ObjectId,
        ref: "User",
        required: true
    },
    enterDate:{
        type: Date,
        required: true,
        default: Date.now
    },
    exitDate:{
        type: Date,
        required: false,
    },
    isAtTheGym:{
        type: Boolean,
        required: true,
    },
    sessionDuration:{
        type: Number,
        required: false
    }
})

export const GymSession = mongoose.model<IgymSession>("GymSession", gymSessionSchema);