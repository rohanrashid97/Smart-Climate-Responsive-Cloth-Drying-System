This is a [Next.js](https://nextjs.org) project bootstrapped with [`create-next-app`](https://github.com/vercel/next.js/tree/canary/packages/create-next-app).

## Getting Started

First, run the development server:

```bash
npm run dev
# or
yarn dev
# or
pnpm dev
# or
bun dev
```

Open [http://localhost:3000](http://localhost:3000) with your browser to see the result.

You can start editing the page by modifying `app/page.js`. The page auto-updates as you edit the file.

This project uses [`next/font`](https://nextjs.org/docs/app/building-your-application/optimizing/fonts) to automatically optimize and load [Geist](https://vercel.com/font), a new font family for Vercel.

## Learn More

To learn more about Next.js, take a look at the following resources:

- [Next.js Documentation](https://nextjs.org/docs) - learn about Next.js features and API.
- [Learn Next.js](https://nextjs.org/learn) - an interactive Next.js tutorial.

You can check out [the Next.js GitHub repository](https://github.com/vercel/next.js) - your feedback and contributions are welcome!

## Deploy on Vercel

🔥 SMART PROTECTION SYSTEM
(ESP8266 + Firebase + Next.js Dashboard)

FINAL STEP-BY-STEP GUIDE

🧠 SYSTEM OVERVIEW (কি বানানো হয়েছে)

এই প্রজেক্টে আমরা বানিয়েছি:

🌡 Temperature & Humidity Monitoring (DHT22)

🌧 Rain Detection

☀️ Day / 🌙 Night Detection (LDR)

🧱 Automatic Protection (Shade + Wiper)

🌐 Web Dashboard (Next.js)

🔐 Login / Register System

👑 Admin Control (Servo control)

👤 Normal User (View only)

☁ Firebase Realtime Database

🚀 Deployable on Vercel

🛠 PART 1: HARDWARE & ARDUINO SETUP
✅ 1️⃣ Required Hardware

NodeMCU ESP8266

DHT22 (Temperature + Humidity)

LDR (Light Sensor)

Rain Sensor Module

2× Servo Motor

Jumper wires

External power (for servo)

✅ 2️⃣ Arduino IDE Setup
Install Libraries (Arduino IDE → Library Manager)

Install these:

ESP8266WiFi

Servo

DHT sensor library

ArduinoJson

WiFiClientSecure

✅ 3️⃣ Upload FINAL ESP8266 Code

👉 আমরা final version ব্যবহার করেছি
👉 Logic change নাই, শুধু documented

✔ Code features:

Local protection logic (Internet না থাকলেও কাজ করবে)

Firebase থেকে control পড়বে

Firebase-এ sensor data পাঠাবে

📌 এই code একবার upload করলেই ESP side complete

☁ PART 2: FIREBASE SETUP (MOST IMPORTANT)
✅ 4️⃣ Firebase Project Create

Go to Firebase Console

Create new project

Enable Realtime Database

Enable Authentication → Email/Password

✅ 5️⃣ Firebase Database Structure (FINAL)
{
  "sensors": {
    "temperature": 21.7,
    "humidity": 76.2,
    "rain": false,
    "ldr": 0
  },
  "controls": {
    "system": true,
    "servo1": true,
    "servo2": false
  },
  "users": {
    "USER_UID": {
      "role": "admin"
    }
  }
}


📌 Explanation:

sensors → ESP writes

controls → Admin dashboard writes

users → role-based access (admin / user)

✅ 6️⃣ Make Yourself ADMIN

Firebase → Authentication → Users

Copy your UID

Database → users → UID

Set:

role: "admin"

💻 PART 3: DASHBOARD (NEXT.JS + VS CODE)
✅ 7️⃣ Install Required Software
🔹 Install Node.js (LTS)

👉 https://nodejs.org

Check installation:

node -v
npm -v

✅ 8️⃣ Create Next.js Project

Open VS Code

Open Terminal inside VS Code:
View → Terminal


Run:

npx create-next-app@latest iot-dashboard


Choose:

App Router → YES

Tailwind → YES

Import alias → @/*

🛑 IMPORTANT: VS Code Terminal Commands
🔹 Stop running server
Ctrl + C

🔹 Restart server
npm run dev


👉 এইটা খুব important — env change করলে সবসময় restart করতে হবে

✅ 9️⃣ Firebase SDK Install
cd iot-dashboard
npm install firebase

✅ 🔟 Environment Variables (.env.local)

Create file:

.env.local


Add:

NEXT_PUBLIC_FIREBASE_API_KEY=xxxx
NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN=xxxx
NEXT_PUBLIC_FIREBASE_DATABASE_URL=xxxx
NEXT_PUBLIC_FIREBASE_PROJECT_ID=xxxx


⚠️ Rules:

No quotes

No commas

Restart server after this

✅ 1️⃣1️⃣ Firebase Config File

📄 src/lib/firebase.js

Exports:

auth

db

Used everywhere (login, register, dashboard).

🔐 PART 4: AUTHENTICATION FLOW (FINAL)
✅ 1️⃣2️⃣ Pages Structure (FINAL)
/login
/register
/dashboard
/  → redirect to /login

✅ 1️⃣3️⃣ Login Page

Firebase Auth

Email + Password

Redirects to /dashboard

✅ 1️⃣4️⃣ Register Page

Creates new user

Saves role = "user" in database

Redirects to dashboard

✅ 1️⃣5️⃣ Role-Based Control (MOST IMPORTANT)
Admin:

See system & servo buttons

Control hardware

User:

Only view sensors + weather

No control buttons visible

👉 Role read from:

users/UID/role

🧭 PART 5: DASHBOARD LOGIC
✅ 1️⃣6️⃣ Dashboard Features

Live sensor data

Day / Night logic same as ESP

Weather forecast (Open-Meteo, no API key)

Logout button

Admin-only controls

✅ 1️⃣7️⃣ Fixes Applied (FINAL)

✔ Root redirect / → /login
✔ Hydration error fixed (suppressHydrationWarning)
✔ Env keys hidden for Vercel
✔ Role-based UI rendering
✔ No 404 on deploy

🚀 PART 6: VERCEL DEPLOYMENT
✅ 1️⃣8️⃣ Before Deploy Checklist

✔ .env.local in .gitignore
✔ Firebase keys removed from code
✔ Env vars added in Vercel dashboard
✔ npm run dev works locally

✅ 1️⃣9️⃣ Deploy to Vercel
npm install -g vercel
vercel


Add same env variables in:

Vercel → Settings → Environment Variables

The easiest way to deploy your Next.js app is to use the [Vercel Platform](https://vercel.com/new?utm_medium=default-template&filter=next.js&utm_source=create-next-app&utm_campaign=create-next-app-readme) from the creators of Next.js.

Check out our [Next.js deployment documentation](https://nextjs.org/docs/app/building-your-application/deploying) for more details.
