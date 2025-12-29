"use client";

import { auth, db } from "@/lib/firebase";
import { onAuthStateChanged, signOut } from "firebase/auth";
import { get, onValue, ref, set } from "firebase/database";
import { useRouter } from "next/navigation";
import { useEffect, useState } from "react";

export default function Dashboard() {
  const [sensors, setSensors] = useState({});
  const [controls, setControls] = useState({});
  const [city, setCity] = useState("Dhaka");
  const [weather, setWeather] = useState(null);
  const router = useRouter();
  const [role, setRole] = useState(null);

  useEffect(() => {
  const unsub = onAuthStateChanged(auth, async (user) => {
    if (!user) {
      router.push("/login");
      return;
    }

    const snap = await get(ref(db, `users/${user.uid}`));
    if (snap.exists()) {
      setRole(snap.val().role);
    }
  });

  return () => unsub();
}, []);

  useEffect(() => {
    onValue(ref(db, "sensors"), snap => {
      setSensors(snap.val());
    });

    onValue(ref(db, "controls"), snap => {
      setControls(snap.val());
    });
  }, []);



  /* ---------- WEATHER FETCH (NO API KEY) ---------- */
  useEffect(() => {
    const fetchWeather = async () => {
      try {
        // 1️⃣ Geocode city → lat/lon
        const geoRes = await fetch(
          `https://nominatim.openstreetmap.org/search?q=${encodeURIComponent(
            city
          )}&format=json&limit=1`
        );
        const geoData = await geoRes.json();
        if (!geoData.length) return;

        const { lat, lon } = geoData[0];

        // 2️⃣ Fetch forecast from Open-Meteo
        const weatherRes = await fetch(
          `https://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}&hourly=temperature_2m,precipitation_probability&timezone=auto`
        );
        const weatherData = await weatherRes.json();

        setWeather({
          temp: weatherData.hourly.temperature_2m[0],
          rainProb: weatherData.hourly.precipitation_probability[0],
          time: weatherData.hourly.time[0],
        });
      } catch (err) {
        console.error("Weather error:", err);
      }
    };

    fetchWeather();
  }, [city]);

  const toggle = (key, value) => {
    set(ref(db, `controls/${key}`), value);
  };

  return (
    <main className="min-h-screen bg-gray-100 p-8">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-3xl font-bold">🌐 IoT Protection Dashboard</h1>

        <button
          onClick={() => {
            signOut(auth);
            router.push("/login");
          }}
          className="bg-red-600 text-white px-4 py-2 rounded"
        >
          Logout
        </button>
      </div>

      {/* Sensor Cards */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4 mb-8">
        <Card title="Temperature" value={`${sensors?.temperature ?? "--"} °C`} />
        <Card title="Humidity" value={`${sensors?.humidity ?? "--"} %`} />
        <Card title="Rain" value={sensors?.rain ? "YES 🌧" : "NO ☀️"} />
        <Card
          title="Time"
          value={
            sensors?.ldr === undefined
              ? "--"
              : sensors.ldr <= 300
                ? "DAY ☀️"
                : "NIGHT 🌙"
          }
        />
      </div>

      {/* Controls */}
      {role === "admin" && (
  <section title="Controls">
    <Toggle
      label="System"
      value={controls.system}
      onClick={() => toggle("system")}
    />
    <Toggle
      label="Servo 1"
      value={controls.servo1}
      onClick={() => toggle("servo1")}
    />
    <Toggle
      label="Servo 2"
      value={controls.servo2}
      onClick={() => toggle("servo2")}
    />
  </section>
)}
      {/* WEATHER */}

      <section title="🌦Weather Forecast (Free)">
        <input
          value={city}
          onChange={(e) => setCity(e.target.value)}
          className="border p-2 rounded w-full mb-4"
          placeholder="Enter city (e.g. Dhaka)"
        />
        {weather ? (
          <div className="space-y-2">
            <p><strong>Forecast Time:</strong> {weather.time}</p>
            <p><strong>Temperature:</strong> {weather.temp} °C</p>
            <p>
              <strong>Rain Probability:</strong>{" "}
              {weather.rainProb}% {weather.rainProb > 50 ? "🌧" : "☀️"}
            </p>
          </div>
        ) : (
          <p>Loading weather…</p>
        )}
      </section>

    </main>
  );
}

function Card({ title, value }) {
  return (
    <div className="bg-white p-4 rounded shadow text-center">
      <p className="text-gray-500">{title}</p>
      <p className="text-xl font-bold">{value}</p>
    </div>
  );
}

function Toggle({ label, value, onClick }) {
  return (
    <div className="flex justify-between items-center mb-4">
      <span>{label}</span>
      <button
        onClick={onClick}
        className={`px-4 py-2 rounded text-white ${value ? "bg-green-600" : "bg-red-600"
          }`}
      >
        {value ? "ON" : "OFF"}
      </button>
    </div>
  );
}
