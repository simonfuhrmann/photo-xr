import { html, css, LitElement } from 'lit';
import { customElement, property, state } from 'lit/decorators.js';

import 'oxygen-mdc/oxy-button'

import * as webXR from '../webxr/webxr'

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

  @property({ type: String }) private path = '';
  @state() private xrAvailable = false;

  override connectedCallback() {
    super.connectedCallback();
    this.checkXR();
  }

  override render() {
    return html`
      ${this.renderXRStatus()}
      ${this.renderXRButton()}
      <div>Image path: ${this.path}</div>
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

  private renderXRButton() {
    if (!this.xrAvailable) return;
    return html`
      <div id="buttons">
        <oxy-button @click=${this.enterXR}>Enter VR</oxy-button>
        <oxy-button @click=${this.leaveXR}>Leave VR</oxy-button>
      </div>
    `;
  }

  private async checkXR() {
    this.xrAvailable = await isWebXRAvailable();
  }

  private async enterXR() {
    if (!navigator.xr) return;
    const session = await navigator.xr.requestSession('immersive-vr', {
      // TODO: Check what I need here.
      requiredFeatures: ['local-floor'],
    });
    webXR.startSession(session);
    console.log('XR session started', session);
  }

  private async leaveXR() {
    webXR.endSession();
  }
}

async function isWebXRAvailable() {
  if (navigator.xr) {
    return await navigator.xr.isSessionSupported('immersive-vr');
  }
  return false;
}
