import 'dotenv/config';
import http from 'node:http';
import crypto from 'node:crypto';
import { MongoClient } from 'mongodb';

const port = Number(process.env.PORT || 8787);
const mongoUri = process.env.MONGODB_URI;
const dbName = process.env.MONGODB_DB || 'deskpal';
const accessToken = process.env.DESKPAL_CLOUD_TOKEN || '';
const allowedOrigins = (process.env.CORS_ORIGIN || '*').split(',').map((item) => item.trim());
const MAX_BODY = 32 * 1024;
const MIN_BASELINE_SAMPLES = 20;
const BASELINE_HOURS = 24;

if (!mongoUri || !accessToken || accessToken.length < 24) {
  console.error('Set MONGODB_URI and a DESKPAL_CLOUD_TOKEN of at least 24 characters in cloud/.env.');
  process.exit(1);
}

const client = new MongoClient(mongoUri, { serverSelectionTimeoutMS: 10000 });
let collections;

function send(res, status, body) {
  res.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store' });
  res.end(JSON.stringify(body));
}

function cors(req, res) {
  const origin = req.headers.origin;
  const allowAny = allowedOrigins.includes('*');
  if (origin && !allowAny && !allowedOrigins.includes(origin)) return false;
  if (origin) res.setHeader('Access-Control-Allow-Origin', allowAny ? '*' : origin);
  res.setHeader('Vary', 'Origin');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, X-DeskPal-Cloud-Token');
  return true;
}

function authorized(req) {
  const candidate = Buffer.from(String(req.headers['x-deskpal-cloud-token'] || ''));
  const expected = Buffer.from(accessToken);
  return candidate.length === expected.length && crypto.timingSafeEqual(candidate, expected);
}

async function bodyJson(req) {
  let raw = '';
  for await (const chunk of req) {
    raw += chunk;
    if (raw.length > MAX_BODY) throw Object.assign(new Error('Request body is too large.'), { status: 413 });
  }
  try { return JSON.parse(raw || '{}'); }
  catch { throw Object.assign(new Error('Request body must be valid JSON.'), { status: 400 }); }
}

function validDeviceId(value) {
  return typeof value === 'string' && /^[a-zA-Z0-9_-]{1,48}$/.test(value);
}

function finiteOrNull(value, min, max) {
  return typeof value === 'number' && Number.isFinite(value) && value >= min && value <= max ? value : null;
}

function median(rows, field) {
  const values = rows.map((row) => row[field]).filter(Number.isFinite).sort((a, b) => a - b);
  if (!values.length) return null;
  const middle = Math.floor(values.length / 2);
  return values.length % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2;
}

async function calculateDecisions(deviceId, observedAt, current) {
  const cutoff = new Date(observedAt.getTime() - BASELINE_HOURS * 60 * 60 * 1000);
  const history = await collections.telemetry.find({
    deviceId,
    observedAt: { $gte: cutoff, $lt: observedAt },
    sensorsOk: true,
    temperatureC: { $type: 'number' }
  }, { projection: { temperatureC: 1, humidityPct: 1 } }).sort({ observedAt: -1 }).limit(2000).toArray();

  const temperatureBaseline = median(history, 'temperatureC');
  const humidityBaseline = median(history, 'humidityPct');
  const desired = [];
  if (history.length >= MIN_BASELINE_SAMPLES && current.sensorsOk) {
    if (current.temperatureC !== null && temperatureBaseline !== null) {
      const delta = current.temperatureC - temperatureBaseline;
      if (Math.abs(delta) >= 2) desired.push({
        kind: delta > 0 ? 'temperature-high' : 'temperature-low',
        message: `Room is ${Math.abs(delta).toFixed(1)} °C ${delta > 0 ? 'warmer' : 'cooler'} than its recent usual level.`,
        suggestion: 'Check the room thermometer and keep the BME280 away from warm boards and trapped heat.',
        baseline: Number(temperatureBaseline.toFixed(1)), current: current.temperatureC
      });
    }
    if (current.humidityPct !== null && humidityBaseline !== null) {
      const delta = current.humidityPct - humidityBaseline;
      if (Math.abs(delta) >= 12) desired.push({
        kind: delta > 0 ? 'humidity-high' : 'humidity-low',
        message: `Humidity is ${Math.abs(delta).toFixed(0)} points ${delta > 0 ? 'higher' : 'lower'} than its recent usual level.`,
        suggestion: delta > 0 ? 'Consider airing out the room.' : 'If the room feels dry, consider adding moisture.' ,
        baseline: Number(humidityBaseline.toFixed(1)), current: current.humidityPct
      });
    }
  }

  for (const kind of ['temperature-high', 'temperature-low', 'humidity-high', 'humidity-low']) {
    const item = desired.find((candidate) => candidate.kind === kind);
    await collections.decisions.updateOne({ deviceId, kind }, { $set: {
      deviceId, kind, active: Boolean(item),
      message: item?.message || `The ${kind.startsWith('temperature') ? 'temperature' : 'humidity'} is back near its recent usual level.`,
      suggestion: item?.suggestion || '', baseline: item?.baseline ?? null, current: item?.current ?? null,
      updatedAt: observedAt
    } }, { upsert: true });
  }
  const decisions = await collections.decisions.find({ deviceId, active: true }).sort({ updatedAt: -1 }).toArray();
  return {
    learning: history.length < MIN_BASELINE_SAMPLES,
    sampleCount: history.length,
    minimumSamples: MIN_BASELINE_SAMPLES,
    baselineHours: BASELINE_HOURS,
    decisions: decisions.map(({ kind, message, suggestion, baseline, current: value, updatedAt }) => ({ kind, message, suggestion, baseline, current: value, updatedAt }))
  };
}

const server = http.createServer(async (req, res) => {
  if (!cors(req, res)) return send(res, 403, { error: 'This dashboard origin is not allowed.' });
  if (req.method === 'OPTIONS') { res.writeHead(204); return res.end(); }
  const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
  if (req.method === 'GET' && url.pathname === '/health') {
    try { await client.db(dbName).command({ ping: 1 }); return send(res, 200, { ok: true, mongoReady: true }); }
    catch { return send(res, 503, { ok: false, mongoReady: false }); }
  }
  if (!authorized(req)) return send(res, 401, { error: 'Invalid cloud access token.' });

  try {
    if (req.method === 'POST' && url.pathname === '/api/telemetry') {
      const input = await bodyJson(req);
      if (!validDeviceId(input.deviceId)) return send(res, 400, { error: 'deviceId must use 1–48 letters, numbers, underscores, or dashes.' });
      const parsedTime = new Date(input.observedAt || Date.now());
      const observedAt = Number.isNaN(parsedTime.getTime()) ? new Date() : parsedTime;
      const sample = {
        deviceId: input.deviceId,
        observedAt,
        sensorsOk: input.sensorsOk === true,
        temperatureC: finiteOrNull(input.temperatureC, -40, 85),
        humidityPct: finiteOrNull(input.humidityPct, 0, 100),
        pressureHpa: finiteOrNull(input.pressureHpa, 300, 1200),
        dewPointC: finiteOrNull(input.dewPointC, -80, 80),
        heatIndexC: finiteOrNull(input.heatIndexC, -80, 100),
        comfortScore: finiteOrNull(input.comfortScore, 0, 100),
        moldRisk: ['LOW', 'MEDIUM', 'HIGH'].includes(input.moldRisk) ? input.moldRisk : null,
        tempTrendCPerHr: finiteOrNull(input.tempTrendCPerHr, -100, 100),
        humTrendPerHr: finiteOrNull(input.humTrendPerHr, -1000, 1000),
        pressTrendHpaPerHr: finiteOrNull(input.pressTrendHpaPerHr, -100, 100),
        pressureForecast: ['Learning', 'Fair weather', 'No change', 'Rain possible', 'Storm risk'].includes(input.pressureForecast) ? input.pressureForecast : null,
        temperaturePrediction: typeof input.temperaturePrediction === 'string' ? input.temperaturePrediction.slice(0, 31) : null,
        inferred: typeof input.inferred === 'string' ? input.inferred.slice(0, 31) : null,
        mood: typeof input.mood === 'string' ? input.mood.slice(0, 24) : 'UNKNOWN',
        comfort: typeof input.comfort === 'string' ? input.comfort.slice(0, 24) : 'UNKNOWN',
        anomaly: input.anomaly === true
      };
      const learning = await calculateDecisions(input.deviceId, observedAt, sample);
      const prior = await collections.telemetry.findOne({ deviceId: input.deviceId }, { sort: { observedAt: -1 }, projection: { observedAt: 1 } });
      const stored = !prior || observedAt.getTime() - prior.observedAt.getTime() >= 55000;
      if (stored) await collections.telemetry.insertOne(sample);
      return send(res, 200, { ok: true, stored, ...learning });
    }
    if (req.method === 'POST' && url.pathname === '/api/feedback') {
      const input = await bodyJson(req);
      const requiredFields = ['deviceId', 'at', 'label', 'kind', 'temperatureC', 'humidityPct'];
      if (!input || typeof input !== 'object' || Array.isArray(input) ||
          Object.keys(input).length !== requiredFields.length ||
          requiredFields.some((field) => !Object.prototype.hasOwnProperty.call(input, field))) {
        return send(res, 400, { error: 'Provide only deviceId, at, label, kind, temperatureC, and humidityPct.' });
      }
      if (!validDeviceId(input.deviceId)) return send(res, 400, { error: 'deviceId must use 1-48 letters, numbers, underscores, or dashes.' });
      if (typeof input.at !== 'string') return send(res, 400, { error: 'at must be a valid ISO timestamp.' });
      const at = new Date(input.at);
      if (Number.isNaN(at.getTime()) || at.toISOString() !== input.at)
        return send(res, 400, { error: 'at must be a valid ISO timestamp.' });
      if (!['cold', 'hot', 'perfect'].includes(input.label) || !['temp', 'hum'].includes(input.kind))
        return send(res, 400, { error: 'label must be cold, hot, or perfect; kind must be temp or hum.' });
      if (finiteOrNull(input.temperatureC, -40, 85) === null ||
          finiteOrNull(input.humidityPct, 0, 100) === null)
        return send(res, 400, { error: 'temperatureC and humidityPct must be finite sensor readings in range.' });

      await collections.feedback.insertOne({
        deviceId: input.deviceId,
        at,
        label: input.label,
        kind: input.kind,
        temperatureC: input.temperatureC,
        humidityPct: input.humidityPct
      });
      return send(res, 201, { ok: true });
    }
    if (req.method === 'POST' && url.pathname === '/api/events') {
      const input = await bodyJson(req);
      if (!validDeviceId(input.deviceId) || !input.event || typeof input.event.msg !== 'string') return send(res, 400, { error: 'A deviceId and event message are required.' });
      const eventDoc = {
        deviceId: input.deviceId,
        receivedAt: new Date(),
        eventId: Number.isSafeInteger(input.event.id) ? input.event.id : null,
        uptimeMs: Number.isSafeInteger(input.event.uptimeMs) ? input.event.uptimeMs : null,
        source: typeof input.event.source === 'string' ? input.event.source.slice(0, 24) : 'Device',
        message: input.event.msg.slice(0, 180)
      };
      if (eventDoc.eventId !== null && eventDoc.uptimeMs !== null) {
        await collections.events.updateOne(
          { deviceId: eventDoc.deviceId, eventId: eventDoc.eventId, uptimeMs: eventDoc.uptimeMs },
          { $setOnInsert: eventDoc },
          { upsert: true }
        );
      } else await collections.events.insertOne(eventDoc);
      return send(res, 201, { ok: true });
    }
    if (req.method === 'GET' && url.pathname === '/api/summary') {
      const deviceId = url.searchParams.get('deviceId');
      if (!validDeviceId(deviceId)) return send(res, 400, { error: 'A valid deviceId is required.' });
      const rawHours = url.searchParams.get('hours') || '24';
      const hours = Number(rawHours);
      if (!Number.isInteger(hours) || hours < 1 || hours > 168) return send(res, 400, { error: 'hours must be an integer from 1 to 168.' });
      const cutoff = new Date(Date.now() - hours * 60 * 60 * 1000);
      const sampleMatch = { deviceId, observedAt: { $gte: cutoff }, sensorsOk: true, temperatureC: { $type: 'number' } };
      const [totals, hourly, inferredEventCount] = await Promise.all([
        collections.telemetry.aggregate([
          { $match: sampleMatch },
          { $group: {
            _id: null,
            sampleCount: { $sum: 1 },
            temperatureMin: { $min: '$temperatureC' }, temperatureMax: { $max: '$temperatureC' }, temperatureAverage: { $avg: '$temperatureC' },
            humidityMin: { $min: '$humidityPct' }, humidityMax: { $max: '$humidityPct' }, humidityAverage: { $avg: '$humidityPct' },
            comfortSampleCount: { $sum: { $cond: [{ $isNumber: '$comfortScore' }, 1, 0] } },
            comfortSamples: { $sum: { $cond: [{ $gte: ['$comfortScore', 70] }, 1, 0] } }
          } },
          { $project: { _id: 0, sampleCount: 1, temperatureMin: 1, temperatureMax: 1, temperatureAverage: 1,
            humidityMin: 1, humidityMax: 1, humidityAverage: 1,
            comfortScore70Percent: { $cond: [{ $gt: ['$comfortSampleCount', 0] }, { $multiply: [{ $divide: ['$comfortSamples', '$comfortSampleCount'] }, 100] }, null] } } }
        ]).toArray(),
        collections.telemetry.aggregate([
          { $match: sampleMatch },
          { $group: { _id: { $dateToString: { format: '%Y-%m-%dT%H:00:00.000Z', date: '$observedAt', timezone: 'UTC' } }, averageTemperatureC: { $avg: '$temperatureC' } } }
        ]).toArray(),
        collections.events.countDocuments({ deviceId, source: 'Inference', receivedAt: { $gte: cutoff } })
      ]);
      const result = totals[0] || { sampleCount: 0, temperatureMin: null, temperatureMax: null, temperatureAverage: null,
        humidityMin: null, humidityMax: null, humidityAverage: null, comfortScore70Percent: null };
      hourly.sort((a, b) => a.averageTemperatureC - b.averageTemperatureC);
      const hourResult = (row) => row ? { hour: row._id, averageTemperatureC: Number(row.averageTemperatureC.toFixed(1)) } : null;
      return send(res, 200, {
        ok: true, deviceId, hours, sampleCount: result.sampleCount,
        temperatureC: { min: result.temperatureMin, max: result.temperatureMax, average: result.temperatureAverage },
        humidityPct: { min: result.humidityMin, max: result.humidityMax, average: result.humidityAverage },
        comfortScore70Percent: result.comfortScore70Percent,
        hottestHour: hourResult(hourly[hourly.length - 1]), coolestHour: hourResult(hourly[0]),
        inferredEventCount
      });
    }
    if (req.method === 'GET' && url.pathname === '/api/decisions/latest') {
      const deviceId = url.searchParams.get('deviceId');
      if (!validDeviceId(deviceId)) return send(res, 400, { error: 'A valid deviceId is required.' });
      const cutoff = new Date(Date.now() - BASELINE_HOURS * 60 * 60 * 1000);
      const sampleCount = await collections.telemetry.countDocuments({ deviceId, observedAt: { $gte: cutoff }, sensorsOk: true, temperatureC: { $type: 'number' } });
      const decisions = await collections.decisions.find({ deviceId, active: true }).sort({ updatedAt: -1 }).toArray();
      return send(res, 200, { ok: true, learning: sampleCount < MIN_BASELINE_SAMPLES, sampleCount, minimumSamples: MIN_BASELINE_SAMPLES, baselineHours: BASELINE_HOURS, decisions: decisions.map(({ kind, message, suggestion, baseline, current, updatedAt }) => ({ kind, message, suggestion, baseline, current, updatedAt })) });
    }
    return send(res, 404, { error: 'Not found.' });
  } catch (error) {
    console.error('Request failed:', error.message);
    return send(res, error.status || 500, { error: error.status ? error.message : 'Cloud request failed.' });
  }
});

try {
  await client.connect();
  const db = client.db(dbName);
  collections = { telemetry: db.collection('telemetry'), events: db.collection('events'), decisions: db.collection('decisions'), feedback: db.collection('feedback') };
  await Promise.all([
    collections.telemetry.createIndex({ deviceId: 1, observedAt: -1 }),
    collections.events.createIndex({ deviceId: 1, receivedAt: -1 }),
    collections.events.createIndex({ deviceId: 1, eventId: 1, uptimeMs: 1 }, { unique: true, partialFilterExpression: { eventId: { $type: 'number' }, uptimeMs: { $type: 'number' } } }),
    collections.decisions.createIndex({ deviceId: 1, kind: 1 }, { unique: true }),
    collections.feedback.createIndex({ deviceId: 1, at: -1 })
  ]);
  server.listen(port, '0.0.0.0', () => console.log(`DeskPal cloud memory listening on port ${port}`));
} catch (error) {
  console.error('Could not connect to MongoDB. Check MONGODB_URI and Atlas network access:', error.message);
  process.exit(1);
}

for (const signal of ['SIGINT', 'SIGTERM']) process.on(signal, async () => {
  server.close(); await client.close(); process.exit(0);
});
