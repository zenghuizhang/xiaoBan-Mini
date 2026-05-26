export const TECH_TOKENS = {
  bg:        '#000000',
  panel:     '#0a1e28',
  accent:    '#22D3EE',
  accent_hi: '#33C5FF',
  text:      '#b4ebff',
  border:    '#14506e',
  danger:    '#F43F5E',
} as const;

export const LAVENDER_TOKENS = {
  bg:        '#FAF5FF',
  panel:     '#FFFFFF',
  accent:    '#9333EA',
  accent_hi: '#A855F7',
  text:      '#4C1D95',
  border:    '#E9D5FF',
  danger:    '#EF4444',
} as const;

export const CHILD_TOKENS = {
  bg:        '#FFF9E6',
  panel:     '#FFFFFF',
  accent:    '#FF7F50',
  accent_hi: '#FFAA78',
  text:      '#c85a28',
  border:    '#FFC8A0',
  danger:    '#F43F5E',
} as const;

export const COCOA_TOKENS = {
  bg:        '#2D1B0E',
  panel:     '#3F2B20',
  accent:    '#FB7185',
  accent_hi: '#FDA4AF',
  text:      '#FEF3C7',
  border:    '#573D2C',
  danger:    '#EF4444',
} as const;

export type ThemeName = 'tech' | 'lavender' | 'child' | 'cocoa';
export const TOKENS_TABLE = {
  tech: TECH_TOKENS,
  lavender: LAVENDER_TOKENS,
  child: CHILD_TOKENS,
  cocoa: COCOA_TOKENS,
} as const;