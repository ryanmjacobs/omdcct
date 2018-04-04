#!/usr/bin/env node

const STAGGER_SIZE = 10;

// database
const low = require("lowdb");
const FileSync = require("lowdb/adapters/FileSync");
const adapter = new FileSync("db.json");
const db = low(adapter);
db.defaults({
    // scoops
    scoops: default_scoops(),

    // jobs
    iter: 0,
    finished: [],
    running: []
}).write();

// webserver
const Koa = require("koa");
const app = new Koa();
const logger = require("koa-logger");
const bodyparser = require("koa-bodyparser");

app.use(logger());
app.use(bodyparser());

app.use(async ctx => {
    const p = ctx.request.body;
    const req = ctx.request.method + ctx.url;

    // next plot requested
    if (req == "GET/next") {
        const {iter,snonce,STAGGER_SIZE} = next();

        // create running job
        db.get("running").push({
            iter, start: new Date()
        }).write();

        // return parameters to the user
        ctx.body = `${iter},${snonce},${STAGGER_SIZE}`;
    }

    // plot completion
    else if (req == "POST/done") {
        const iter = parseInt(p.iter);
        const job  = db.get("running").find({iter}).value();

        if (!job) {
            ctx.status = 404;
            ctx.body = `iter ${iter} not found`;
            return;
        }

        // push finished plot
        job.end = new Date();
        job.google_drive_id = p.google_drive_id;
        db.get("finished").push(job).write();

        // remove from running
        db.get("running").remove({iter}).write();
        ctx.body = `iter ${iter} marked as done`;
    }

    // plot failed
    else if (req == "POST/fail") {
        const iter = parseInt(p.iter);
        db.get("running").remove({iter}).write();
        ctx.body = "removed iter " + iter;
    }

    // client requests for a scoop to be locked,
    // so they can append and upload their changes
    else if (req == "POST/lock") {
        const ref = `scoops[${parseInt(p.scoop)}]`;
        const scoop = db.get(ref).value();

        // scoop already locked
        if (scoop.locked) {
            ctx.body = `scoop #${p.scoop} is already locked`;
            ctx.status = 409;
            return;
        }

        // lock and return scoop link
        scoop.locked = true;
        db.set(ref, scoop).write();
        ctx.body = `${scoop.link}`;
    }

    // Set new scoop link, then unlock it so that
    // other clients can lock, append, and upload
    else if (req == "POST/unlock") {
        const ref = `scoops[${parseInt(p.scoop)}]`;
        const scoop = db.get(ref).value();

        console.log(ref);

        // only locked scoops can be unlocked
        if (!scoop.locked) {
            ctx.body = `refusing to set link for unlocked scoop[${p.scoop}]`;
            ctx.status = 409;
            return;
        }

        // write link and unlock
        scoop.link = p.link;
        scoop.locked = false;
        db.set(ref, scoop).write();
        ctx.body = `successfully set scoop[${p.scoop}].link to '${p.link}'`;
    }

    else if (req == "GET/health-check")
        ctx.status = 200;
    else
        return ctx.status = 404;
});

function serialize(obj) {
    let str = "";
    for (const key of Object.keys(obj))
        str += ` ${key}=${obj[key]}`;
    return str;
}

function next() {
    const iter = db.get("iter").value();

    db.update("iter", x => x+1).write();

    const snonce = iter*STAGGER_SIZE;
    return {iter,snonce,STAGGER_SIZE};
}

function default_scoops() {
    return (new Array(4096))
        .fill({locked:false, link:null});
}

app.listen(3745, function() {
    console.log("listening http://0.0.0.0:3745");
});
