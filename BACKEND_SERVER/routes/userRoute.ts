import {NextFunction, Request, Response, Router} from 'express';
import {IUser} from "../models/user";
import {User} from "../models/user";
import {userInfo} from "node:os";
import {GymSession} from "../models/gymSession";


const router = Router();

// save user to database

router.post("/register", async (req: Request, res: Response) => {
    console.log("connection !!!!!")
    try{
        const {name, surname,email,gymMembershipStarts,gymMembershipEnds,coffeePoints,card_UID} = req.body;

        const newUser = new User({
            name,
            surname,
            email,
            gymMembershipStarts,
            gymMembershipEnds,
            coffeePoints,
            card_UID
        })
        await newUser.save();
        res.status(200).json({ status: "success", message: "User successfully added." });

    }
    catch(err:any){
        res.status(500).json({ status: "error", message: err.message });

    }
})

async function verifyGymMembership(req: Request, res: Response, next: NextFunction) {
    try {
        const card_UID =req.params.card_UID;

        const user = await User.findOne({card_UID: card_UID })

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

        res.locals.user = user;
        next();
    } catch (err: any) {
        if (err.name === 'CastError') {
            return res.status(400).json({ status: "error", message: "Invalid ID format" });
        }
        return res.status(500).json({
            status: "error",
            message: "Internal server error: " + err.message
        });
    }
}

// check is gym membership is valid
router.get("/getMembership/:userID",verifyGymMembership, async (req: Request, res: Response) => {
    return res.status(200).json(res.locals.user);
    })

router.post('/add/coffee/points/:card_ID/:points', async (req: Request, res: Response) => {
    try{
        const card_ID:string = req.params.card_ID;
        const pointsToAdd:number = Number(req.params.points);

        const user = await User.findOne({card_ID:card_ID})

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

router.post('/subtract/coffee/points/:card_ID/:points', async (req: Request, res: Response) => {
    try{
        const card_ID:string = req.params.card_ID;
        const pointsToSubtract:number = Number(req.params.points);

        const user = await User.findOne({card_ID:card_ID})

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
router.post("/enter/exit/gym/:card_UID",verifyGymMembership, async (req: Request, res: Response) => {
    try{
        const card_UID: string = req.params.card_UID;

        const user = await User.findOne({ card_UID: card_UID });

        if (!user) {
            return res.status(404).json({
                status: "error",
                message: "No user found with this card UID"
            });
        }

        let newSession = await GymSession.findOne({
            user:card_UID,
            isAtTheGym: true
        })

        if (!newSession) {
                newSession = new GymSession({
                user:card_UID,
                isAtTheGym: true
            })
            await newSession.save();

            res.status(201).json({
                status: "success",
                message: "Gym entry recorded",
                session: newSession
            });
        }
        else{
            const exitDate:Date = new Date();
            const sessionDurationInDates = exitDate.getTime() - newSession.enterDate.getTime()
            const sessionDurationInHours= Number((sessionDurationInDates / (1000 * 60 * 60)).toFixed(2));

            newSession.exitDate = exitDate;
            newSession.sessionDuration = sessionDurationInHours;
            newSession.isAtTheGym = false;

            await newSession.save();

            return res.status(200).json({
                status: "success",
                message: "Exit recorded"
            });
        }
    }
    catch (err: any) {
        if (err.name === 'CastError') {
            return res.status(400).json({ status: "error", message: "Invalid User ID format" });
        }
        return res.status(500).json({
            status: "error",
            message: "Internal server error: " + err.message
        });
    }
})

router.post("/change/membershipState/:card_UID/:state", async (req: Request, res: Response) => {
    try {
        const user = res.locals.user;
        const gymMembershipState: string = req.params.state;

        const now = new Date();

        if (gymMembershipState === "0") {
            user.gymMembershipStarts = now;

            const endDate = new Date();
            endDate.setMonth(endDate.getMonth() + 3);

            user.gymMembershipEnds = endDate;
        } else {
            user.gymMembershipEnds = now;
        }
        await user.save();

        return res.status(200).json({
            status: "success",
            message: `Membership state updated to ${gymMembershipState === "0" ? "ACTIVE" : "INACTIVE"}`,
            user: user
        });

    } catch (err: any) {
        return res.status(500).json({
            status: "error",
            message: "Internal server error: " + err.message
        });
    }
});

router.get("/getUserData/:card_UID/", async (req: Request, res: Response) => {
    try {
        const card_UID: string = req.params.card_UID;

        const user = await User.findOne({ card_UID: card_UID });

        if (!user) {
            return res.status(404).json({
                status: "error",
                message: "No user found with this card UID"
            });
        }

        const sessions = await GymSession.find({ user: user._id }).sort({ enterDate: -1 });

        return res.status(200).json({
            status: "success",
            user: user,
            sessions: sessions
        });

    } catch (err: any) {
        return res.status(500).json({
            status: "error",
            message: "Internal server error: " + err.message
        });
    }
});

export default router;