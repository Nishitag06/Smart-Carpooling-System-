import React, { useEffect, useState } from "react";

const blankDriver = {
  name: "", origin: "JIIT Sector 62, Noida", destination: "JIIT Sector 128, Noida",
  departureTime: "08:00", availableSeats: 2, maxWaitMinutes: 10, maxDetourMinutes: 25
};
const blankPassenger = {
  name: "", pickup: "Noida Sector 59, Noida", dropoff: "JIIT Sector 128, Noida",
  preferredPickupTime: "08:20", earlyToleranceMinutes: 10, lateToleranceMinutes: 15, seatsNeeded: 1
};

async function request(url, options = {}) {
  const response = await fetch(url, {
    headers: { "Content-Type": "application/json" },
    ...options
  });
  const data = await response.json();
  if (!response.ok) throw new Error(data.error || "Request failed");
  return data;
}

const asArray = value => Array.isArray(value) ? value : [];

export default function App() {
  const [locations, setLocations] = useState([]);
  const [roadData, setRoadData] = useState({ averageSpeedKmh: 30 });
  const [drivers, setDrivers] = useState([]);
  const [passengers, setPassengers] = useState([]);
  const [bookings, setBookings] = useState([]);
  const [driverForm, setDriverForm] = useState(blankDriver);
  const [passengerForm, setPassengerForm] = useState(blankPassenger);
  const [matches, setMatches] = useState([]);
  const [selectedPassenger, setSelectedPassenger] = useState("");
  const [selectedDriver, setSelectedDriver] = useState("");
  const [mode, setMode] = useState("passenger");
  const [message, setMessage] = useState("");
  const [error, setError] = useState("");
  const [bookingConfirmation, setBookingConfirmation] = useState(null);

  async function refresh() {
    const [l, d, p, b, roadInfo] = await Promise.all([
      request("/api/locations"), request("/api/drivers"),
      request("/api/passengers"), request("/api/bookings"),
      request("/api/roads")
    ]);
    const locationList = asArray(l);
    const driverList = asArray(d);
    const passengerList = asArray(p);
    setLocations(locationList); setDrivers(driverList); setPassengers(passengerList); setBookings(asArray(b));
    setRoadData({ averageSpeedKmh: roadInfo.averageSpeedKmh });
    if (!selectedPassenger && passengerList.length) setSelectedPassenger(String(passengerList[0].id));
    if (!selectedDriver && driverList.length) setSelectedDriver(String(driverList[0].id));
  }

  useEffect(() => { refresh().catch(e => setError(e.message)); }, []);

  async function submitDriver(e) {
    e.preventDefault(); setError(""); setMessage(""); setBookingConfirmation(null);
    try {
      await request("/api/drivers", { method: "POST", body: JSON.stringify(driverForm) });
      setMessage("Driver registered successfully.");
      setDriverForm(blankDriver); await refresh();
    } catch (e) { setError(e.message); }
  }
  async function submitPassenger(e) {
    e.preventDefault(); setError(""); setMessage(""); setBookingConfirmation(null);
    try {
      await request("/api/passengers", { method: "POST", body: JSON.stringify(passengerForm) });
      setMessage("Passenger registered successfully.");
      setPassengerForm(blankPassenger); await refresh();
    } catch (e) { setError(e.message); }
  }
  async function searchMatches() {
    setError(""); setMessage(""); setBookingConfirmation(null); setMatches([]);
    try {
      if (mode === "passenger") {
        if (!selectedPassenger) throw new Error("Select a passenger first.");
        setMatches(asArray(await request(`/api/matches/passenger/${selectedPassenger}`)));
      } else {
        if (!selectedDriver) throw new Error("Select a driver first.");
        setMatches(asArray(await request(`/api/matches/driver/${selectedDriver}`)));
      }
    } catch (e) { setError(e.message); }
  }
  async function book(match) {
    setError(""); setMessage(""); setBookingConfirmation(null);
    try {
      const result = await request("/api/bookings", {
        method: "POST",
        body: JSON.stringify({ driverId: match.driverId, passengerId: match.passengerId })
      });
      setMessage(result.message); setBookingConfirmation(result.booking); setMatches([]); await refresh();
    } catch (e) { setError(e.message); }
  }
  async function cancel(id) {
    setError(""); setBookingConfirmation(null);
    try {
      const result = await request(`/api/bookings/${id}`, { method: "DELETE" });
      setMessage(result.message); await refresh();
    } catch (e) { setError(e.message); }
  }

  const update = (setter, key) => e => setter(old => ({ ...old, [key]: e.target.value }));
  const locationSelect = (value, onChange) => (
    <select value={value} onChange={onChange}>
      {locations.map(place => <option key={place} value={place}>{place}</option>)}
    </select>
  );
  const activeBookings = bookings.filter(b => b.active);

  return <main className="shell">
    <header className="hero">
      <div className="eyebrow">SMART MOBILITY • ROUTE SHARING</div>
      <h1>Smart <span>Carpool</span></h1>
      <p>Find people travelling in a similar direction. Share a ride, reduce traffic, and keep the environment greener.</p>
      <img className="hero-eco" src="/carpool-eco.svg" alt="Green carpool car with trees" />
      <div className="stats">
        <div><strong>{drivers.length}</strong><small>Drivers</small></div>
        <div><strong>{passengers.length}</strong><small>Passengers</small></div>
        <div><strong>{activeBookings.length}</strong><small>Active bookings</small></div>
      </div>
      <div className="api-status"><i /> API connected · localhost:18080</div>
    </header>

    <section className="panel roads-panel">
      <div className="panel-heading"><span className="number">ROAD</span><div><h2>Carpool road network</h2><p>Actual routed paths are shown with each proposed and accepted ride.</p></div></div>
      <p className="road-speed">Estimated-time speed assumption: <b>{roadData.averageSpeedKmh} km/h</b> where road-specific speed data is unavailable. Distances are not displayed.</p>
    </section>

    {(message || error) && <div className={error ? "notice error" : "notice"}>{error || message}</div>}
    {bookingConfirmation && <section className="booking-confirmation" aria-live="polite">
      <strong>Ride booked successfully!</strong>
      <span>Driver: {bookingConfirmation.driverName} · {bookingConfirmation.pickup} → {bookingConfirmation.dropoff} · {bookingConfirmation.departureTime} · {bookingConfirmation.status}</span>
    </section>}

    <section className="forms-grid">
      <form className="panel" onSubmit={submitDriver}>
        <div className="panel-heading"><span className="number">01</span><div><h2>Offer a ride</h2><p>Register your planned journey</p></div></div>
        <label>Your name<input required value={driverForm.name} onChange={update(setDriverForm, "name")} placeholder="Enter driver name" /></label>
        <div className="two">
          <label>Starting point{locationSelect(driverForm.origin, update(setDriverForm, "origin"))}</label>
          <label>Destination{locationSelect(driverForm.destination, update(setDriverForm, "destination"))}</label>
        </div>
        <div className="two">
          <label>Departure time<input type="time" required value={driverForm.departureTime} onChange={update(setDriverForm, "departureTime")} /></label>
          <label>Available seats<input type="number" min="1" max="8" value={driverForm.availableSeats} onChange={update(setDriverForm, "availableSeats")} /></label>
        </div>
        <div className="two">
          <label>Max wait (minutes)<input type="number" min="0" value={driverForm.maxWaitMinutes} onChange={update(setDriverForm, "maxWaitMinutes")} /></label>
          <label>Max detour (minutes)<input type="number" min="0" value={driverForm.maxDetourMinutes} onChange={update(setDriverForm, "maxDetourMinutes")} /></label>
        </div>
        <button className="primary" type="submit">Register as driver <span>↗</span></button>
      </form>

      <form className="panel" onSubmit={submitPassenger}>
        <div className="panel-heading"><span className="number">02</span><div><h2>Find a ride</h2><p>Tell us where you need to go</p></div></div>
        <label>Your name<input required value={passengerForm.name} onChange={update(setPassengerForm, "name")} placeholder="Enter passenger name" /></label>
        <div className="two">
          <label>Pickup point{locationSelect(passengerForm.pickup, update(setPassengerForm, "pickup"))}</label>
          <label>Drop-off point{locationSelect(passengerForm.dropoff, update(setPassengerForm, "dropoff"))}</label>
        </div>
        <div className="two">
          <label>Preferred pickup<input type="time" required value={passengerForm.preferredPickupTime} onChange={update(setPassengerForm, "preferredPickupTime")} /></label>
          <label>Seats needed<input type="number" min="1" max="8" value={passengerForm.seatsNeeded} onChange={update(setPassengerForm, "seatsNeeded")} /></label>
        </div>
        <div className="two">
          <label>Can be earlier by (min)<input type="number" min="0" value={passengerForm.earlyToleranceMinutes} onChange={update(setPassengerForm, "earlyToleranceMinutes")} /></label>
          <label>Can be later by (min)<input type="number" min="0" value={passengerForm.lateToleranceMinutes} onChange={update(setPassengerForm, "lateToleranceMinutes")} /></label>
        </div>
        <button className="primary alt" type="submit">Register as passenger <span>↗</span></button>
      </form>
    </section>

    <section className="panel match-panel">
      <div className="panel-heading"><span className="number">03</span><div><h2>Match journeys</h2><p>Search from either side of the carpool</p></div></div>
      <div className="search-controls">
        <div className="mode-switch">
          <button type="button" className={mode === "passenger" ? "selected" : ""} onClick={() => { setMode("passenger"); setMatches([]); }}>I need a ride</button>
          <button type="button" className={mode === "driver" ? "selected" : ""} onClick={() => { setMode("driver"); setMatches([]); }}>I have a car</button>
        </div>
        {mode === "passenger"
          ? <label>Select passenger<select value={selectedPassenger} onChange={e => setSelectedPassenger(e.target.value)}>{passengers.map(p => <option key={p.id} value={p.id}>#{p.id} · {p.name} ({p.pickup} → {p.dropoff})</option>)}</select></label>
          : <label>Select driver<select value={selectedDriver} onChange={e => setSelectedDriver(e.target.value)}>{drivers.map(d => <option key={d.id} value={d.id}>#{d.id} · {d.name} ({d.origin} → {d.destination})</option>)}</select></label>}
        <button className="primary search-button" onClick={searchMatches}>Find compatible journeys <span>⌕</span></button>
      </div>

      {matches.length > 0 && <div className="results">
        <div className="results-title"><h3>{matches.length} compatible match{matches.length > 1 ? "es" : ""}</h3><small>Ranked by lowest additional time, then lowest driver wait</small></div>
        {matches.map((m, index) => {
          const driver = drivers.find(d => d.id === m.driverId);
          const passenger = passengers.find(p => p.id === m.passengerId);
          const route = asArray(m.route);
          const originalRoute = asArray(m.originalRoute);
          return <article className="match-card" key={`${m.driverId}-${m.passengerId}`}>
            <div className="match-top"><span className="match-index">MATCH 0{index + 1}</span><strong>{driver?.name || `Driver #${m.driverId}`} <span className="muted">↔</span> {passenger?.name || `Passenger #${m.passengerId}`}</strong><span className="detour">+{m.detourDistanceKm.toFixed(2)} km detour</span></div>
            <div className="route-caption">Driver's original route</div>
            <div className="route-line original-route">{originalRoute.map((stop, i) =>
              <React.Fragment key={`original-${stop}-${i}`}>
                <span className="stop">{stop}</span>
                {i < originalRoute.length - 1 && <span className="route-arrow original-step">→</span>}
              </React.Fragment>
            )}</div>
            <div className="route-caption">Shared journey · estimated road path</div>
            <div className="route-legend">
              <span><i className="legend-original" />Original route</span>
              <span><i className="legend-detour" />Detour to meeting point</span>
              <span><i className="legend-shared" />Shared journey</span>
            </div>
            <div className="route-line journey-route">{route.map((stop, i) => {
              const roles = [];
              if (i === 0) roles.push("Driver starts");
              if (i === m.pickupRouteIndex) roles.push("Meeting point");
              if (i === m.passengerDestinationRouteIndex) roles.push("Passenger destination");
              if (i === route.length - 1) roles.push("Driver's destination");
              const stopClass = [
                "stop",
                i === m.pickupRouteIndex ? "meeting-point" : "",
                i === m.passengerDestinationRouteIndex ? "passenger-destination" : "",
                i === route.length - 1 ? "driver-destination" : ""
              ].filter(Boolean).join(" ");
              return <React.Fragment key={`${stop}-${i}`}>
                <span className={stopClass}>{roles.length > 0 && <small>{roles.join(" · ")}</small>}{stop}</span>
                {i < route.length - 1 && <span className={`route-arrow ${i < m.pickupRouteIndex ? "detour-step" : "shared-step"}`}>→</span>}
              </React.Fragment>;
            })}</div>
            <div className="metrics">
              <div><small>Estimated meeting-point arrival</small><b>{m.pickupArrivalTime}</b></div>
              <div><small>Passenger pickup window</small><b>{m.passengerEarliestPickupTime}–{m.passengerLatestPickupTime}</b><em className={m.pickupArrivalWithinWindow ? "time-in-window" : "time-before-window"}>{m.pickupArrivalWithinWindow ? "Arrival is within window" : "Early; driver waits before pickup"}</em></div>
              <div><small>Driver wait before window</small><b>{m.driverWaitMinutes} min</b></div>
              <div><small>Passenger wait by arrival</small><b>{m.passengerWaitByArrivalMinutes} min</b><em>After requested {passenger?.preferredPickupTime || "pickup time"}</em></div>
              <div><small>Original route distance</small><b>{m.baseRouteDistanceKm.toFixed(2)} km</b></div>
              <div><small>Additional detour distance</small><b>{m.detourDistanceKm.toFixed(2)} km</b></div>
              <div><small>Estimated additional time</small><b>{m.detourMinutes} min</b></div>
              <div><small>Total estimated shared time</small><b>{m.sharedRouteMinutes} min</b></div>
            </div>
            <button className="book-button" onClick={() => book(m)}>Confirm this booking</button>
          </article>;
        })}
      </div>}
      {matches.length === 0 && <p className="empty">No results displayed yet. Register a journey, choose a passenger or driver, then search.</p>}
    </section>

    <section className="panel bookings-panel">
      <div className="panel-heading"><span className="number">04</span><div><h2>Shared bookings</h2><p>Rides booked by everyone in this running session</p></div></div>
      {bookings.length === 0 ? <p className="empty">No shared bookings yet.</p> : bookings.map(b => {
        const route = asArray(b.route);
        return <div className="booking-row" key={b.id}>
          <div>
            <b>Booking #{b.id} · {b.status}</b>
            <span>{b.passengerName} with {b.driverName} · {b.pickup} → {b.dropoff} · {b.departureTime} · {b.seatsBooked} seat(s)</span>
            {route.length > 0 && <>
              <span className="route-caption">Accepted shared path</span>
              <div className="route-line booking-route">{route.map((stop, i) => <React.Fragment key={`${stop}-${i}`}><span className="stop">{stop}</span>{i < route.length - 1 && <span className="route-arrow">→</span>}</React.Fragment>)}</div>
            </>}
          </div>
          {b.active ? <button className="cancel" onClick={() => cancel(b.id)}>Cancel</button> : <span className="cancelled">Cancelled</span>}
        </div>;
      })}
    </section>
    <footer><span>SMART CARPOOL</span><span>Built with C++ · Crow · React</span></footer>
  </main>;
}
