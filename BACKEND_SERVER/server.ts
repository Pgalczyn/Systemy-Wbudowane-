import express from "express";
import {connectDatabase} from "./database";
import userRoute from "./routes/userRoute";
const PORT = 3000;
const app = express();


app.use(express.json());
app.use("/", userRoute);

async function startServer() {
    try{
        await connectDatabase();
        app.listen(PORT, () => {
            console.log(`Server started on port ${PORT}`);

        })
    }catch(err:any){
        console.error(err.message + "server did not start on port " + PORT);
    }
}

startServer();