# Feature: Mushin Website Hosting

This document details the configuration, local development, and deployment workflows for the static **Mushin Product Website** using **Firebase Hosting**.

---

## 1. Overview & Architecture

The Mushin product website is a high-performance, single-page marketing application. To deliver an ultra-responsive user experience that aligns with our premium product design, the site is deployed as a static bundle hosted on Firebase's global Content Delivery Network (CDN).

* **Source Directory:** `site/` (located in the project root)
* **Hosting Platform:** Firebase Hosting (CDN-backed, HTTPS-by-default, HTTP/2 multiplexing)
* **Assets:** Custom modular styling (`style.css`), interactive hero canvas shader (`index.html`), and high-resolution visuals.

---

## 2. Configuration Files

To make the codebase Firebase-ready out-of-the-box, the following configuration files have been established in the project root:

### `firebase.json`
Specifies that the `site/` folder contains the public assets, and excludes configuration/meta-files from being uploaded:
```json
{
  "hosting": {
    "public": "site",
    "ignore": [
      "firebase.json",
      "**/.*",
      "**/node_modules/**"
    ]
  }
}
```

### `.firebaserc`
Associates the codebase with the default Firebase project:
```json
{
  "projects": {
    "default": "mushin-audio"
  }
}
```

---

## 3. Operations Workflow

### Step 1: Install Firebase CLI
Install the Firebase command-line tools globally via `npm`:
```powershell
npm install -g firebase-tools
```

### Step 2: Authentication
Authenticate the CLI tool with your Google/Firebase account:
```powershell
firebase login
```

### Step 3: Local Testing & Emulation
To run a local server and test the website exactly as it will behave in production (including custom headers and rewrites):
```powershell
# Start the local Hosting emulator
firebase emulators:start --only hosting
```
Once running, open your browser and navigate to **`http://localhost:5000`** to interact with the website locally.

### Step 4: Production Deployment
To build, package, and upload the static assets in `site/` directly to production:
```powershell
firebase deploy --only hosting
```
Your website will immediately become live at `https://mushin-audio.web.app` (and `https://mushin-audio.firebaseapp.com`), or on your custom domain if mapped in the Firebase Console.

---

## 4. Performance & Custom Domain Setup

### Custom Domain Mapping
To point a custom domain (e.g., `mushin.audio` or `getmushin.com`) to your Firebase site:
1. Navigate to the **Firebase Console** -> **Hosting** section.
2. Click **Add Custom Domain**.
3. Enter your domain name and follow the instructions to configure your DNS records (TXT record for ownership verification, followed by A records pointing to Firebase's CDN IP addresses).
4. Firebase will automatically provision a free SSL certificate via Let's Encrypt within 24 hours.

### Cache Configuration (Optional)
To further optimize load speeds for global audiences, cache control headers can be configured in `firebase.json`:
```json
{
  "hosting": {
    "public": "site",
    "headers": [
      {
        "source": "assets/**",
        "headers": [
          {
            "key": "Cache-Control",
            "value": "max-age=31536000"
          }
        ]
      }
    ]
  }
}
```