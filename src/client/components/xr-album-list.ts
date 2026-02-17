import { html, css, LitElement, nothing } from 'lit';
import { customElement, property, state } from 'lit/decorators.js';

import 'oxygen-mdc/oxy-icon';
import 'oxygen-mdc/oxy-icons-base'
import 'oxygen-mdc/oxy-icons-image'

import '../icons/oxy-icons-xr'
import * as api from '../modules/server_api';
import * as types from '../modules/server_types';

@customElement('xr-album-list')
export class XrAlbumList extends LitElement {
  static styles = css`
    :host {
      display: flex;
      flex-direction: column;
    }
    xr-album-list {
      margin-left: 24px;
    }
    .entry {
      display: flex;
      flex-direction: row;
      align-items: center;
      cursor: pointer;
      padding: 2px 8px;
      margin: 1px;
      border-radius: 3px;
      user-select: none;
    }
    .entry[selected] {
      background-color: rgba(255, 255, 255, 0.1);
    }
    .entry oxy-icon {
      margin-right: 4px;
      transform: rotate(0deg);
      transition: transform 0.2s ease;
    }
    .entry oxy-icon[open] {
      transform: rotate(90deg);
    }
    .entry oxy-icon[spin] {
      animation: spin 2s linear infinite;
    }

    @keyframes spin {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }
  `;

  @property({ type: String }) path = '';
  @state() private selected = '';
  @state() private albumLoading = '';
  @state() private apiError?: api.HttpError;
  @state() private apiData?: types.AlbumResponse;

  override connectedCallback(): void {
    super.connectedCallback();
    this.onRequest();
  }

  override render() {
    return html`
      ${this.renderError()}
      ${this.renderData()}
    `;
  }

  private renderError() {
    if (!this.apiError) return nothing;
    return html`<div class="error">Error: ${this.apiError.message}</div>`;
  }

  private renderData() {
    if (!this.apiData) return nothing;
    return this.apiData.entries.map((entry) => this.renderEntry(entry));
  }

  private renderEntry(entry: types.AlbumEntry) {
    if (entry.type === types.EntryType.PHOTO) {
      return this.renderPhotoEntry(entry);
    } else if (entry.type === types.EntryType.ALBUM) {
      return this.renderAlbumEntry(entry);
    }
    return nothing;
  }

  private renderPhotoEntry(entry: types.AlbumEntry) {
    const isOpen = entry.name === this.selected;
    const onClick = () => {
      this.selected = entry.name;
      const detail = { path: `${this.path}/${entry.name}` };
      this.dispatchEvent(new CustomEvent('photo-selected', { detail, bubbles: true, composed: true }));
    }
    const icon = getIconForEntry(entry);
    return html`
      <div class="entry" ?selected=${isOpen} @click=${onClick}>
        <oxy-icon icon="${icon}"></oxy-icon>
        <div>${entry.name}</div>
      </div>
    `;
  }

  private renderAlbumEntry(entry: types.AlbumEntry) {
    const isOpen = entry.name === this.selected;
    const onClick = () => {
      this.selected = isOpen ? '' : entry.name;
    }
    const isLoading = entry.name === this.albumLoading;
    const icon = isLoading ? 'icons:autorenew' : 'icons:chevron-right';
    return html`
      <div class="entry" ?selected=${isOpen} @click=${onClick}>
        <oxy-icon ?open=${isOpen} ?spin=${isLoading} icon="${icon}"></oxy-icon>
        <div>${entry.name}</div>
      </div>
      ${isOpen ? this.renderSubAlbum(entry) : nothing}
    `;
  }

  private renderSubAlbum(entry: types.AlbumEntry) {
    const entryPath = `${this.path}/${entry.name}`;
    const onAlbumLoading = (e: CustomEvent<boolean>) => {
      this.albumLoading = e.detail ? entry.name : '';
    }
    return html`
      <xr-album-list
        path="${this.path}/${entry.name}"
        @loading=${onAlbumLoading}>
      </xr-album-list>
    `;
  }

  private onRequest() {
    // Set a timeout after which to show a loading spinner.
    // Clear the timeout when the request finishes earlier.
    const timeout = setTimeout(() => {
      this.dispatchEvent(new CustomEvent('loading', { detail: true }));
    }, 250);

    this.apiData = undefined
    api.getAlbum(this.path).then((data) => {
      this.apiData = data;
    }).catch((err: api.HttpError) => {
      this.apiError = err;
    }).finally(() => {
      clearTimeout(timeout);
      this.dispatchEvent(new CustomEvent('loading', { detail: false }));
    });
  }
}

function getIconForEntry(entry: types.AlbumEntry) {
  if (entry.stereo === 'gphoto') {
    return 'xr:vr180';
  } else if (entry.stereo === 'sbs') {
    return 'xr:sbs';
  }
  return 'image:photo';
}
