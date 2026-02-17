import { html, css, LitElement } from 'lit';
import { customElement, state } from 'lit/decorators.js';

import './xr-app-sidebar';
import './xr-app-viewer';

@customElement('xr-app')
export class XrApp extends LitElement {
  static styles = css`
    :host {
      width: 100vw;
      height: 100vh;
      display: flex;
      flex-direction: row;
    }
    xr-app-sidebar {
      flex-shrink: 0;
    }
    xr-app-viewer {
      flex-grow: 1;
    }
  `;

  @state() private selectedPath = '';

  override render() {
    const onSelected = (e: CustomEvent<{ path: string }>) => {
      this.selectedPath = e.detail.path;
    };
    return html`
      <xr-app-sidebar @photo-selected=${onSelected}></xr-app-sidebar>
      <xr-app-viewer .path="${this.selectedPath}"></xr-app-viewer>
    `;
  }
}
