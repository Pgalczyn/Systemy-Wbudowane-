import { Request, Response,Router } from 'express';
import {IUser} from "../models/user";
import {User} from "../models/user";
import {userInfo} from "node:os";
import {GymSession} from "../models/gymSession";


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

// check is gym membership is valid
router.get("/getMembership/:userID", async (req: Request, res: Response) => {
    try {
        const userID:string = req.params.userID;

        const user = await User.findById(userID)

        if (!user) {
            return res.status(404).json({
                status: "error",
                message: "User with such an ID does not exist"
            });
        }
        const now = new Date();
        const isActive = user.gymMembershipStarts <= now && user.gymMembershipEnds >= now;

        if (!isActive) {
            return res.status(403).json({
                status: "error",
                message: "Membership is expired or not yet active"
            });
        }

        res.status(200).json(user);
    }
    catch (err: any) {
        if (err.name === 'CastError') {
            return res.status(400).json({ status: "error", message: "Invalid ID format" });
        }
        res.status(500).json({
            status: "error",
            message: "Internal server error: " + err.message
        });
    }

    })

router.post('/add/coffee/points/:userID/:points', async (req: Request, res: Response) => {
    try{
        const userID:string = req.params.userID;
        const pointsToAdd:number = Number(req.params.points);

        const user = await User.findById(userID)

        if (!user) {
            return res.status(404).json({
                status: "error",
                message: "User with such an ID does not exist"
            });
        }

        if (isNaN(pointsToAdd)) {
            return res.status(400).json({ status: "error", message: "Points must be a number" });
        }

        user.coffeePoints += pointsToAdd;
        res.status(200).json(user);
    }
    catch (err: any) {
        if (err.name === 'CastError') {
            return res.status(400).json({ status: "error", message: "Invalid ID format" });
        }
        res.status(500).json({
            status: "error",
            message: "Internal server error: " + err.message
        });
    }
})

router.post('/subtract/coffee/points/:userID/:points', async (req: Request, res: Response) => {
    try{
        const userID:string = req.params.userID;
        const pointsToSubtract:number = Number(req.params.points);

        const user = await User.findById(userID)

        if (!user) {
            return res.status(404).json({
                status: "error",
                message: "User with such an ID does not exist"
            });
        }

        if (isNaN(pointsToSubtract)) {
            return res.status(400).json({ status: "error", message: "Points must be a number" });
        }

        user.coffeePoints += pointsToSubtract;
        res.status(200).json(user);
    }
    catch (err: any) {
        if (err.name === 'CastError') {
            return res.status(400).json({ status: "error", message: "Invalid ID format" });
        }
        res.status(500).json({
            status: "error",
            message: "Internal server error: " + err.message
        });
    }
})

//enter gym
router.post("/enter/gym/:userID", async (req: Request, res: Response) => {
    try{
        const userID:string = req.params.userID;


        const newSession = new GymSession({
            user:userID,
            isAtTheGym: true
        })

        await newSession.save();

        res.status(201).json({
            status: "success",
            message: "Gym entry recorded",
            session: newSession
        });
    }
    catch (err: any) {
        if (err.name === 'CastError') {
            return res.status(400).json({ status: "error", message: "Invalid User ID format" });
        }
        res.status(500).json({
            status: "error",
            message: "Internal server error: " + err.message
        });
    }
})
// exit gym

router.patch("/exit/gym/:userID", async (req: Request, res: Response) => {
    try{
        const userID:string = req.params.userID;

        const newSession = await GymSession.findOne({
            user:userID,
            isAtTheGym: true
        })
        if (!newSession) {
            return res.status(404).json({ status: "error", message: "No active session found" });
        }

        const exitDate:Date = new Date();
        const sessionDurationInDates = exitDate.getTime() - newSession.enterDate.getTime()
        const sessionDurationInHours= Number((sessionDurationInDates / (1000 * 60 * 60)).toFixed(2));

        newSession.exitDate = exitDate;
        newSession.sessionDuration = sessionDurationInHours;
        newSession.isAtTheGym = false;

        await newSession.save()

        res.status(200).json({
            status: "success",
            message: "Exit recorded"
        });
    }
    catch (err: any) {
        res.status(500).json({ status: "error", message: err.message });
    }

})

export default router;