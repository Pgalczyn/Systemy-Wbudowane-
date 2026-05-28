import mongoose from "mongoose";
import * as process from "node:process";

export async function connectDatabase() {
    try{
        const mongoURI: string = 'mongodb://localhost:27017/embedded';

        await mongoose.connect(mongoURI);
        console.log("MongoDB Connected Successfully");
    }
    catch(e: any){
        console.error(e.message);

        process.exit(1);
    }
}