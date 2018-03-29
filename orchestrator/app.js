#!/usr/bin/env node

const NONCES = 40960;

// database
const low = require("lowdb");
const FileSync = require("lowdb/adapters/FileSync");
const adapter = new FileSync("db.json");
const db = low(adapter);
db.defaults({
    pid: 0,
    iter: 0,

    aof: [],

    completed_plots: [],
    ongoing_plots: []
}).write();

// webserver
const Koa = require("koa");
const bodyParser = require("koa-bodyparser");
const app = new Koa();
app.use(bodyParser());

app.use(async (ctx, next) => {
    console.log(ctx.method, ctx.url);
    await next();
});

app.use(async ctx => {
    const p = ctx.request.body;
    const req = ctx.request.method + ctx.url;

    // next plot requested
    if (req == "GET/next") {
        const {pid,iter,snonce,nonces} = next();

        // create ongoing job
        db.get("ongoing_plots").push({
            started: new Date(),
            pid, iter
        }).write();

        // return parameters to the user
        ctx.body = `${pid},${snonce},${nonces}`;
    }

    // plot completion
    else if (req == "POST/complete") {
        const pid = parseInt(p.pid);
        const job = db.get("ongoing_plots").find({pid}).value();

        if (!job) {
            ctx.status = 404;
            ctx.body = `pid ${pid} not found`;
            return;
        }

        // push completed plot
        job.end = new Date();
        job.google_drive_id = p.google_drive_id;
        db.get("completed_plots").push(job).write();

        // remove from ongoing
        db.get("ongoing_plots").remove({pid}).write();
        ctx.body = "";

    } else if (req == "GET/fail") {
        const pid = parseInt(p.pid);
        db.get("ongoing_plots").remove({pid}).write();
        ctx.body = "removed pid " + pid;
    }

    else if (req == "GET/status")
        ctx.body = "orchestrator";

    else
        return ctx.status = 404;

    db.get("aof").push(req + serialize(p)).write();
});

function serialize(obj) {
    let str = "";
    for (const key of Object.keys(obj))
        str += ` ${key}=${obj[key]}`;
    return str;
}

function next() {
    const pid = db.get("pid").value();
    const iter = db.get("iter").value();

    db.update("pid", x => x+1).write();
    db.update("iter", x => x+1).write();

    const snonce = iter*NONCES;
    return {pid,iter,snonce,nonces};
}

app.listen(3745, function() {
    console.log("listening http://0.0.0.0:3745");
});
