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

app.use(async (ctx,next) => {
    const p = ctx.request.body;
    const req = ctx.request.method + ctx.url;

    // next plot requested
    if (req == "GET/next") {
        const {iter,snonce,STAGGER_SIZE} = next_iter();

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

        // only running jobs can post completion
        if (!job) {
            ctx.status = 404;
            ctx.body = `iter ${iter} not found`;
            next();
            return;
        }

        // push finished plot
        job.end = new Date();
        db.get("finished").push(job).write();

        // remove from running
        db.get("running").remove({iter}).write();
        ctx.body = `iter ${iter} marked as done`;
    }

    // plot failed
    else if (req == "POST/fail") {
        const iter = parseInt(p.iter);

        // only running jobs can fail
        const job = db.get("running").find({iter}).value();
        if (!job) {
            ctx.status = 404;
            ctx.body = `iter ${iter} not found`;
            next();
            return;
        }

        db.get("running").remove({iter}).write();
        ctx.body = "removed iter " + iter;
    }

    // client requests for a scoop to be locked,
    // so they can append and upload their changes
    else if (req == "POST/lock") {
        const iter = parseInt(p.iter);

        // only running jobs can place locks
        const job = db.get("running").find({iter}).value();
        if (!job) {
            ctx.status = 404;
            ctx.body = `iter ${iter} not found`;
            next();
            return;
        }

        const i = parseInt(p.scoop);
        const scoops = db.get("scoops").value();

        purge_locks();

        // scoop already locked
        if (scoops[i].locked) {
            ctx.body = `scoop #${p.scoop} is already locked`;
            ctx.status = 409;
            next();
            return;
        }

        // lock and return scoop link
        scoops[i].locked = iter;
        db.set("scoops", scoops).write();
        ctx.body = `${scoops[i].link}`;
    }

    // Set new scoop link, then unlock it so that
    // other clients can lock, append, and upload
    else if (req == "POST/unlock") {
        console.log(p);

        const iter = parseInt(p.iter);
        const i = parseInt(p.scoop);
        const scoops = db.get("scoops").value();

        // only the owner of a lock can unlock it
        if (scoops[i].locked === false) {
            ctx.body = `scoop[${p.scoop}] is already unlocked`;
            ctx.status = 409;
            next();
            return;
        } else if (iter != scoops[i].locked) {
            ctx.body = `refusing to unlock someone else's lock, for scoop[${p.scoop}]`;
            ctx.status = 409;
            next();
            return;
        }

        // write link and unlock
        scoops[i].link = p.link;
        scoops[i].locked = false;
        db.set("scoops", scoops).write();
        ctx.body = `successfully set scoop[${p.scoop}].link to '${p.link}'`;
    }

    else if (req == "GET/killall") {
        ctx.body = `killing ${db.get("running").value().length} job(s)`;
        db.set("running", []).write();
        ctx.status = 200;
    }

    else if (req == "GET/health-check")
        ctx.status = 200;
    else
        ctx.status = 404;

    next();
});

app.use(async ctx => {
    console.log(
        ctx.response.status  + " "  +
        ctx.response.message + ", " +
        ctx.response.body
    );
});

// unlock all scoops that have expired an owner
function purge_locks() {
    const iters  = db.get("running").map("iter").value();
    const scoops = db.get("scoops").value();

    for (let i = 0; i < scoops.length; i++) {
        const owner = scoops[i].locked;

        if (owner && !iters.includes(owner)) {
            console.log(`purged lock for scoop #${i}`);
            scoops[i].locked = false;
        }
    }
}

function serialize(obj) {
    let str = "";
    for (const key of Object.keys(obj))
        str += ` ${key}=${obj[key]}`;
    return str;
}

function next_iter() {
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
