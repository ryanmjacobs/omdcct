#!/usr/bin/env node

const nonces = 40960;

// database
const low = require("lowdb");
const FileSync = require("lowdb/adapters/FileSync");
const adapter = new FileSync("db.json");
const db = low(adapter);
db.defaults({
    pid: 0,
    iter: 0,

    aof: [],

    completed: [],
    ongoing: []
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

    if (req == "GET/next") {
        const pid = next_pid();
        const snonce = next_snonce();

        // create ongoing job
        db.get("ongoing").push({
            started: new Date(),
            pid, snonce, nonces
        }).write();

        // return parameters to the user
        ctx.body = `${pid},${snonce},${nonces}`;
    } else if (req == "POST/complete") {
        const pid = parseInt(p.pid);
        const job = db.get("ongoing").find({pid}).value();

        if (!job) {
            ctx.status = 404;
            ctx.body = `pid ${pid} not found`;
            return;
        }

        // push job to completed
        job.end = new Date();
        job.google_drive_id = p.google_drive_id;
        db.get("completed").push(job).write();

        // remove from ongoing
        db.get("ongoing").remove({pid}).write();
        ctx.body = "";
    } else if (req == "GET/fail") {
        const pid = parseInt(p.pid);
        db.get("ongoing").remove({pid}).write();
        ctx.body = "removed pid " + pid;
    } else {
        return ctx.status = 404;
    }

    db.get("aof").push(req + serialize(p)).write();
});

function serialize(obj) {
    let str = "";
    for (const key of Object.keys(obj))
        str += ` ${key}=${obj[key]}`;
    return str;
}

function next_pid() {
    db.update("pid", x => x+1).write();
    return db.get("pid").value();
}

function next_snonce() {
    const snonce = db.get("iter").value() * nonces;
    db.update("iter", x => x+1).write();
    return snonce;
}

app.listen(3745, function() {
    console.log("listening http://0.0.0.0:3745");
});
