import { Request, Response,Router } from 'express';
import {IUser} from "../models/user";
import {User} from "../models/user";


const router = Router();

// save user to database

router.post("/register", async (req: Request, res: Response) => {
    try{
        const {name, surname,email,gymMembershipStarts,gymMembershipEnds,coffeePoints} = req.body;

        const newUser = new User({
            name,
            surname,
            email,
            gymMembershipStarts,
            gymMembershipEnds,
            coffeePoints,
        })
        await newUser.save();
        res.status(200).json({ status: "success", message: "User successfully added." });
    }
    catch(err:any){
        res.status(500).json({ status: "error", message: err.message });

    }
})