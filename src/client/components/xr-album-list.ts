import { html, css, LitElement, nothing } from 'lit';
import { customElement, property, state } from 'lit/decorators.js';

import 'oxygen-mdc/oxy-icon';
import 'oxygen-mdc/oxy-icons-base'
import 'oxygen-mdc/oxy-icons-image'

import '../icons/oxy-icons-xr'
import * as api from '../modules/server_api';
import * as types from '../modules/client_types';

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
    switch (entry.type) {
      case types.EntryType.PHOTO:
      case types.EntryType.VIDEO:
        return this.renderMediaEntry(entry);
      case types.EntryType.ALBUM:
        return this.renderAlbumEntry(entry);
      default: break;
    }
    return nothing;
  }

  private renderMediaEntry(entry: types.AlbumEntry) {
    const isOpen = entry.name === this.selected;
    const onMediaSelected = this.onMediaSelected.bind(this, entry);
    return html`
      <div class="entry" ?selected=${isOpen} @click=${onMediaSelected}>
        <oxy-icon icon="${getIconForEntry(entry)}"></oxy-icon>
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
        path="${joinPaths(this.path, entry.name)}"
        @loading=${onAlbumLoading}>
      </xr-album-list>
    `;
  }

  private onMediaSelected(entry: types.AlbumEntry) {
    if (!this.apiData) return;
    this.selected = entry.name;
    const mediaIndex = this.apiData.entries.findIndex((e) => e === entry);
    if (mediaIndex === -1) return;

    const detail = { album: this.apiData, index: mediaIndex };
    const eventOptions = { detail, bubbles: true, composed: true };
    this.dispatchEvent(new CustomEvent('media-selected', eventOptions));
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
  if (entry.type === types.EntryType.VIDEO) {
    return 'image:movie-creation';
  }
  if (entry.stereo === types.StereoMode.GOOGLE_PHOTO) {
    return 'xr:vr180';
  } else if (entry.stereo === types.StereoMode.SIDE_BY_SIDE) {
    return 'xr:sbs';
  }
  return 'image:photo';
}

function joinPaths(path1: string, path2: string) {
  if (path1.endsWith('/') || path2.startsWith('/')) {
    return `${path1}${path2}`;
  }
  return `${path1}/${path2}`;
}
