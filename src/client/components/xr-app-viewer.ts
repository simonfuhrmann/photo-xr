import { html, css, LitElement } from 'lit';
import { customElement, property, state } from 'lit/decorators.js';

import 'oxygen-mdc/oxy-button'

import * as webXR from '../webxr/webxr'
import * as types from '../modules/client_types';

@customElement('xr-app-viewer')
export class XrAppViewer extends LitElement {
  static styles = css`
    :host {
      display: flex;
      flex-direction: column;
      padding: 16px;
    }
    #buttons {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 16px;
      margin: 16px 0;
    }
    #buttons oxy-button {
      padding: 8px 16px;
      font-size: 1.5em;
      background: #6495ED99;
      color: white;
    }
    .warning {
      padding: 8px 16px;
      background-color: rgba(255, 255, 255, 0.1);
      border: 2px solid #950606;
    }
    .success {
      padding: 8px 16px;
      background-color: rgba(255, 255, 255, 0.1);
      border: 2px solid #008000;
    }
  `;

  @property({ attribute: false }) media?: types.SelectedMedia;
  @state() private xrAvailable = false;

  override connectedCallback() {
    super.connectedCallback();
    this.checkXR();
  }

  override render() {
    return html`
      ${this.renderXRStatus()}
      ${this.renderXRButton()}
      ${this.renderMediaInfo()}
    `;
  }

  private renderXRStatus() {
    if (!this.xrAvailable) {
      const secureContext = window.isSecureContext;
      return html`
        <div class="warning">
          WebXR not available,
          ${!secureContext ? html`unsecure HTTP connection.` : ''}
          ${secureContext ? html`device doesn't support WebXR.` : ''}
        </div>`;
    }
    return html`<div class="success">WebXR is available!</div>`;
  }

  private renderMediaInfo() {
    if (!this.media) {
      return html`<div>No media selected.</div>`;
    }

    const entry = this.media.album.entries[this.media.index];
    return html`
      <div>Album: ${this.media.album.path}</div>
      <div>Photo: ${entry.name}</div>
    `;
  }

  private renderXRButton() {
    if (!this.xrAvailable) return;
    return html`
      <div id="buttons">
        <oxy-button
          ?disabled=${!this.media}
          @click=${this.enterXR}>
          Enter VR
        </oxy-button>

        <oxy-button
          ?disabled=${!this.media}
          @click=${this.leaveXR}>
          Leave VR
        </oxy-button>
      </div>
    `;
  }

  private async checkXR() {
    this.xrAvailable = await isWebXRAvailable();
  }

  private async enterXR() {
    if (!navigator.xr || !this.media) return;
    const session = await navigator.xr.requestSession('immersive-vr', {
      requiredFeatures: ['local-floor'], // TODO: Check what I need here.
    });

    // Check if `media` is still valid, because of async function above.
    if (!this.media) return;
    webXR.startSession(session, this.media);
  }

  private async leaveXR() {
    webXR.endSession();
  }
}

async function isWebXRAvailable() {
  if (!navigator.xr) return false;
  return await navigator.xr.isSessionSupported('immersive-vr');
}
