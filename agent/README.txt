Portfolio Agent
===============

The Portfolio Agent is a chat-based tool built into the manage page that allows
you to make edits to your portfolio using natural language. It uses the OpenAI
API key stored in your browser's local storage (never committed to the repo).

How It Works
------------

1. Open the manage page and expand the "Portfolio Agent" section.
2. Type a natural language instruction in the chat input, such as:
   - "Change the site title to My New Portfolio"
   - "Add a project called Dashboard with a description about analytics"
   - "Update the bio to mention five years of backend experience"
   - "Remove the third project"
   - "Change the primary color to dark blue"
3. The agent sends your current portfolio data and your instruction to the
   OpenAI API (using the key from your browser cache).
4. The AI returns an updated version of your portfolio JSON.
5. The agent applies the changes, saves to disk, builds the portfolio, and
   opens a preview in a new browser tab.
6. The agent then asks if you want to deploy the changes live to your
   GitHub Pages site.

Requirements
------------

- An OpenAI API key configured in the AI Content Generation section
  of the manage page (stored only in browser localStorage).
- The portfolio server (serve) must be running.
- Deploy settings configured if you want to deploy live.

Privacy
-------

Your API key is stored only in your browser's localStorage and is sent
directly from your browser to the OpenAI API. It is never stored in any
file in this repository and never passes through the local server.
