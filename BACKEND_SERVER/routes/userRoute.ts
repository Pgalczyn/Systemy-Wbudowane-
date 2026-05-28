import { NextFunction, Request, Response, Router } from 'express';
import { IUser } from "../models/user";
import { User } from "../models/user";
import { GymSession } from "../models/gymSession";

const router = Router();

// Zapis usera do bazy
router.post("/register", async (req: Request, res: Response) => {
    console.log("connection !!!!!");
    try {
        const { name, surname, email, card_UID } = req.body;

        const startDate = new Date();
        const endDate = new Date();
        endDate.setMonth(endDate.getMonth() + 3);

        const newUser = new User({
            name,
            surname,
            email,
            gymMembershipStarts: startDate,
            gymMembershipEnds: endDate,
            coffeePoints: 1000,
            card_UID
        });

        const savedUser = await newUser.save();

        res.status(200).json({
            status: "success",
            message: "User successfully added.",
            user: savedUser
        });

    } catch (error) {
        console.error(error);
        res.status(500).json({ status: "error", message: "Internal server error." });
    }
});

async function verifyGymMembership(req: Request, res: Response, next: NextFunction) {
    try {
        const card_UID = req.params.card_UID;

        const user = await User.findOne({ card_UID: card_UID });

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

// POPRAWKA: Zmiana z :userID na :card_UID, żeby pasowało do middleware!
router.get("/getMembership/:card_UID", verifyGymMembership, async (req: Request, res: Response) => {
    return res.status(200).json(res.locals.user);
});

router.post('/add/coffee/points/:card_UID/:points', async (req: Request, res: Response) => {
    try {
        // POPRAWKA: Ujednoliciłem nazewnictwo na card_UID
        const card_UID: string | string[] = req.params.card_UID;
        const pointsToAdd: number = Number(req.params.points);

        // POPRAWKA: Szukamy po poprawnym kluczu card_UID z modelu!
        const user = await User.findOne({ card_UID: card_UID });

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

        // Zwracamy zapisany stan
        await user.save();

        // Zmieniłem format odpowiedzi na spójny z resztą kodu
        res.status(200).json({
            status: "success",
            user: user
        });
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
});

router.post('/subtract/coffee/points/:card_UID/:points', async (req: Request, res: Response) => {
    try {
        const card_UID: string | string[] = req.params.card_UID;
        const pointsToSubtract: number = Number(req.params.points);

        const user = await User.findOne({ card_UID: card_UID });

        if (!user) {
            return res.status(404).json({
                status: "error",
                message: "User with such an ID does not exist"
            });
        }

        if (isNaN(pointsToSubtract)) {
            return res.status(400).json({ status: "error", message: "Points must be a number" });
        }

        // POPRAWKA: Odejmowanie punktów zamiast dodawania
        user.coffeePoints -= pointsToSubtract;

        await user.save();

        res.status(200).json({
            status: "success",
            user: user
        });
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
});

// Wejście na siłownię
router.post("/enter/exit/gym/:card_UID", verifyGymMembership, async (req: Request, res: Response) => {
    try {
        const card_UID: string | string[] = req.params.card_UID;

        const user = await User.findOne({ card_UID: card_UID });

        if (!user) {
            return res.status(404).json({
                status: "error",
                message: "No user found with this card UID"
            });
        }

        let newSession = await GymSession.findOne({
            user: card_UID,
            isAtTheGym: true
        });

        if (!newSession) {
            newSession = new GymSession({
                user: card_UID,
                isAtTheGym: true
            });
            await newSession.save();

            res.status(201).json({
                status: "success",
                message: "Gym entry recorded",
                session: newSession
            });
        }
        else {
            const exitDate: Date = new Date();
            const sessionDurationInDates = exitDate.getTime() - newSession.enterDate.getTime();
            const sessionDurationInHours = Number((sessionDurationInDates / (1000 * 60 * 60)).toFixed(2));

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
});

// POPRAWKA: Dodałem verifyGymMembership, bo bez tego `res.locals.user` nie istnieje!
router.post("/change/membershipState/:card_UID/:state", verifyGymMembership, async (req: Request, res: Response) => {
    try {
        // Dodałem rzutowanie "as IUser", dzięki czemu TypeScript wie, że to obiekt Usera, i nie podkreśli tego na czerwono
        const user = res.locals.user as IUser;
        const gymMembershipState: string | string[] = req.params.state;

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
        const card_UID: string | string[] = req.params.card_UID;

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