import express, { Request, Response } from "express";
import { User } from "./models/user";
import { GymSession } from "./models/gymSession";

export const router = express.Router();

function unixSeconds(date: Date) {
    return Math.floor(date.getTime() / 1000);
}

router.post('/register', async (req: Request, res: Response) => {
    const { cardUid, name, surname, email } = req.body;

    const startDate = new Date();
    const endDate = new Date();
    endDate.setMonth(endDate.getMonth() + 1);

    const newUser = new User({
        name,
        surname,
        email,
        membershipStart: startDate,
        membershipEnd: endDate,
        points: 100,
        cardUid: cardUid
    });

    let savedUser;
    try {
        savedUser = await newUser.save();
    } catch (error: any) {
        console.error(error);
        return res.status(500).json({ message: "Internal server error: " + (error.message || error) });
    }

    return res.status(200).json({
        userId: savedUser._id.toString(),
        validUntil: unixSeconds(endDate),
        points: savedUser.points
    });
});

router.get('/member/:id', async (req: Request, res: Response) => {
    const id: string = req.params.id;
    const gateFlag = String(req.query.gate || "false").toLowerCase() === 'true';

    let user;
    try {
        user = await User.findById(id);
    } catch (err: any) {
        return res.status(500).json({ message: "Internal server error: " + err.message });
    }

    if (!user) {
        return res.status(404).json({ message: "No user found with this userId" });
    }

    const now = new Date();
    const isActive = user.membershipStart <= now && user.membershipEnd >= now;

    const status = user.membershipState ? user.membershipState : (isActive ? "ACTIVE" : "INACTIVE");

    if (gateFlag) {
        let activeSession;
        try {
            activeSession = await GymSession.findOne({ user: String(user._id), isAtTheGym: true });
        } catch (err: any) {
            return res.status(500).json({ message: "Internal server error: " + err.message });
        }

        if (activeSession) {
            activeSession.exitTime = now;
            activeSession.isAtTheGym = false;

            try {
                await activeSession.save();
            } catch (err: any) {
                return res.status(500).json({ message: "Internal server error: " + err.message });
            }
        } else {
            const newSession = new GymSession({
                user: String(user._id),
                enterTime: now,
                isAtTheGym: true
            });

            try {
                await newSession.save();
            } catch (err: any) {
                return res.status(500).json({ message: "Internal server error: " + err.message });
            }
        }
    }

    return res.status(200).json({
        userId: user._id.toString(),
        validUntil: unixSeconds(new Date(user.membershipEnd)),
        points: user.points,
        status: status
    });
});

router.put('/member/:id/extend-validity', async (req: Request, res: Response) => {
    const id: string = req.params.id;

    let user;
    try {
        user = await User.findById(id);
    } catch (err: any) {
        return res.status(500).json({ message: "Internal server error: " + err.message });
    }

    if (!user) {
        return res.status(404).json({ message: "User not found" });
    }

    const newEnd = new Date(user.membershipEnd);
    newEnd.setMonth(newEnd.getMonth() + 1);
    user.membershipEnd = newEnd;

    try {
        await user.save();
    } catch (err: any) {
        return res.status(500).json({ message: "Internal server error: " + err.message });
    }

    return res.status(200).json({ validUntil: unixSeconds(newEnd) });
});

router.put('/member/:id/points', async (req: Request, res: Response) => {
    const id: string = req.params.id;
    const amount: number = Number(req.body.amount);

    if (isNaN(amount)) {
        return res.status(400).json({ message: "Amount must be a number" });
    }

    let user;
    try {
        user = await User.findById(id);
    } catch (err: any) {
        return res.status(500).json({ message: "Internal server error: " + err.message });
    }

    if (!user) return res.status(404).json({ message: "User not found" });

    user.points = (user.points || 0) + amount;
    try {
        await user.save();
    } catch (err: any) {
        return res.status(500).json({ message: "Internal server error: " + err.message });
    }

    return res.status(200).json({ newTotal: user.points });
});

router.put('/member/:id/state', async (req: Request, res: Response) => {
    const id: string = req.params.id;
    const status: any = req.body?.status;

    if (!status) {
        return res.status(400).json({ message: 'Missing status in request body' });
    }

    if (typeof status !== 'string' || (status !== 'ACTIVE' && status !== 'INACTIVE')) {
        return res.status(400).json({ message: 'Invalid status value. Allowed: "ACTIVE", "INACTIVE"' });
    }

    let user;
    try {
        user = await User.findById(id);
    } catch (err: any) {
        return res.status(500).json({ message: "Internal server error: " + err.message });
    }

    if (!user) return res.status(404).json({ message: "User not found" });
    user.membershipState = status === "ACTIVE" ? "ACTIVE" : "INACTIVE";

    try {
        await user.save();
    } catch (err: any) {
        return res.status(500).json({ message: "Internal server error: " + err.message });
    }

    return res.status(200).json({ status: user.membershipState });
});

export default router;
