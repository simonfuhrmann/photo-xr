import { html, css, LitElement, nothing } from 'lit';
import { customElement, property, state } from 'lit/decorators.js';

import 'oxygen-mdc/oxy-button'
import 'oxygen-mdc/oxy-icon';
import 'oxygen-mdc/oxy-icons-image'
import 'oxygen-mdc/oxy-icons-base'

import '../icons/oxy-icons-xr'
import { webXR } from '../webxr/webxr'
import * as types from '../modules/client_types';

@customElement('xr-app-content')
export class XrAppContent extends LitElement {
  static styles = css`
    :host {
      display: flex;
      flex-direction: column;
      background-color: #111;
    }
    #status {
      display: flex;
      flex-direction: row;
      align-items: center;
      background-color: rgba(255, 255, 255, 0.1);
      padding: 8px 8px 8px 16px;
      gap: 8px;
    }
    #status oxy-icon {
      background-color: rgba(255, 255, 255, 0.1);
      border-radius: 3px;
      padding: 2px;
      background-color: #950606;
    }
    #status .label {
      flex-grow: 1;
    }
    #status[available] oxy-icon {
      background-color: #008000;
    }
    #status oxy-button {
      padding: 8px 16px;
      font-size: 1.5em;
      background: #6495ED99;
      color: white;
    }

    #media-list {
      display: flex;
      flex-direction: column;
      flex-grow: 1;
      padding: 16px;
      gap: 4px;
      overflow-y: scroll;
    }

    div.media {
      display: flex;
      flex-direction: row;
      align-items: center;
      cursor: pointer;

      background-color: rgba(255, 255, 255, 0.1);
      padding: 8px 16px;
      border: 2px solid transparent;
    }
    div.media[selected] {
      border: 2px solid cornflowerblue;
    }
    div.media .media-layout {
      display: flex;
      flex-direction: column;
      flex-grow: 1;
    }
    div.media .info {
      font-size: 0.9em;
      color: #aaa;
    }
    [hidden] {
      display: none !important;
    }
  `;

  private onSessionStarted = () => { this.xrRunning = true; };
  private onSessionEnded = () => { this.xrRunning = false; }

  @property({ attribute: false }) album?: types.AlbumResponse;
  @state() private media?: types.SelectedMedia;
  @state() private xrAvailable = false;
  @state() private xrRunning = false;

  // Push the new selected media into the immersive session, if active.
  override willUpdate(changedProperties: Map<string, unknown>) {
    if (changedProperties.has('album')) {
      this.media = undefined;
    }
    if (changedProperties.has('media')) {
      this.updateMedia();
    }
  }

  override connectedCallback() {
    super.connectedCallback();
    this.checkXR();
    webXR.addEventListener('session-started', this.onSessionStarted);
    webXR.addEventListener('session-ended', this.onSessionEnded);
  }

  override disconnectedCallback() {
    super.disconnectedCallback();
    webXR.removeEventListener('session-started', this.onSessionStarted);
    webXR.removeEventListener('session-ended', this.onSessionEnded);
  }

  override render() {
    return [
      this.renderXRStatus(),
      this.renderAlbumMedia(),
    ];
  }

  private renderXRStatus() {
    const isAvailable = this.xrAvailable;
    const icon = isAvailable ? 'icons:check' : 'icons:warning';
    const label = getStatusLabel(isAvailable);

    return html`
      <div id="status" ?available=${isAvailable}>
        <oxy-icon icon="${icon}"></oxy-icon>
        <div class="label">${label}</div>
        <oxy-button
          ?disabled=${!this.media || !isAvailable}
          ?hidden=${this.xrRunning}
          @click=${this.enterXR}>
          Enter VR
        </oxy-button>

        <oxy-button
          ?disabled=${!this.media || !isAvailable}
          ?hidden=${!this.xrRunning}
          @click=${this.leaveXR}>
          Leave VR
        </oxy-button>
      </div>
    `;
  }

  private renderAlbumMedia() {
    return html`
      <div id="media-list">${this.renderMediaList()}</div>
    `;
  }

  private renderMediaList() {
    if (!this.album) {
      return html`<div>No album selected.</div>`;
    }
    const entries = this.album.entries.filter((entry) => {
      return entry.media !== types.MediaType.ALBUM;
    });
    if (entries.length === 0) {
      return html`<div>No media in album.</div>`;
    }
    return entries.map(this.renderAlbumEntry.bind(this));
  }

  private renderAlbumEntry(entry: types.AlbumEntry, index: number) {
    const isSelected = this.media?.index === index;
    const onSelect = () => {
      if (!this.album) return;
      this.media = { album: this.album, index };
    };
    return html`
      <div class="media" ?selected=${isSelected} @click=${onSelect}>
        <div class="media-layout">
          <div class="filename">${entry.name}</div>
          <div class="info">size: n/a, type: ${entry.media}</div>
        </div>
        <oxy-icon icon=${getIconForMedia(entry)}></oxy-icon>
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

  private updateMedia() {
    if (!this.media) return;
    webXR.updateMedia(this.media);
  }
}

async function isWebXRAvailable() {
  if (!navigator.xr) return false;
  return await navigator.xr.isSessionSupported('immersive-vr');
}

function getStatusLabel(isAvailable: boolean) {
  if (isAvailable) {
    return 'WebXR is available!';
  }
  if (!window.isSecureContext) {
    return 'WebXR not available, unsecure HTTP connection.';
  }
  return 'WebXR not available, unsupported by device.'
}

function getIconForMedia(entry: types.AlbumEntry) {
  if (entry.media === types.MediaType.VIDEO_SBS) {
    return 'image:movie-creation';
  }
  if (entry.media === types.MediaType.IMAGE_GPHOTO) {
    return 'xr:vr180';
  } else if (entry.media === types.MediaType.IMAGE_SBS) {
    return 'xr:sbs';
  }
  return 'image:photo';
}
